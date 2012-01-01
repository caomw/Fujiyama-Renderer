/*
Copyright (c) 2011-2012 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "ObjectInstance.h"
#include "LocalGeometry.h"
#include "Accelerator.h"
#include "Vector.h"
#include "Matrix.h"
#include "Array.h"
#include "Box.h"
#include "Ray.h"
#include <stdlib.h>
#include <assert.h>
#include <float.h>

struct ObjectInstance {
	/* geometric properties */
	const struct Accelerator *acc;
	double bounds[6];

	/* transform properties */
	struct Matrix object_to_world;
	struct Matrix world_to_object;
	double translate[3];
	double rotate[3];
	double scale[3];
	int xform_order;
	int rotate_order;

	/* non-geometric properties */
	const struct Shader *shader;
	const struct Light **target_lights;
	int n_target_lights;
	const struct ObjectGroup *reflection_target;
	const struct ObjectGroup *refraction_target;
};

struct ObjectGroup {
	struct Accelerator *accelerator;
	struct Array *objects;
	double bounds[6];
};

static void update_matrix_and_bounds(struct ObjectInstance *obj);
static void update_group_accelerator(struct ObjectGroup *grp);
static void update_object_bounds(struct ObjectInstance *obj);

static void object_bounds(const void *prim_set, int prim_id, double *bounds);
static int object_ray_intersect(const void *prim_set, int prim_id, const struct Ray *ray,
		struct LocalGeometry *isect, double *t_hit);

/* ObjectInstance interfaces */
struct ObjectInstance *ObjNew(const struct Accelerator *acc)
{
	struct ObjectInstance *obj;

	obj = (struct ObjectInstance *) malloc(sizeof(struct ObjectInstance));
	if (obj == NULL)
		return NULL;

	obj->acc = acc;

	MatIdentity(&obj->object_to_world);
	MatIdentity(&obj->world_to_object);

	VEC3_SET(obj->translate, 0, 0, 0);
	VEC3_SET(obj->rotate, 0, 0, 0);
	VEC3_SET(obj->scale, 1, 1, 1);
	obj->xform_order = ORDER_SRT;
	obj->rotate_order = ORDER_XYZ;

	obj->shader = NULL;
	obj->target_lights = NULL;
	obj->n_target_lights = 0;

	obj->reflection_target = NULL;
	obj->refraction_target = NULL;
	update_matrix_and_bounds(obj);

	return obj;
}

void ObjFree(struct ObjectInstance *obj)
{
	if (obj == NULL)
		return;
	free(obj);
}

void ObjSetTranslate(struct ObjectInstance *obj, double tx, double ty, double tz)
{
	VEC3_SET(obj->translate, tx, ty, tz);
	update_matrix_and_bounds(obj);
}

void ObjSetRotate(struct ObjectInstance *obj, double rx, double ry, double rz)
{
	VEC3_SET(obj->rotate, rx, ry, rz);
	update_matrix_and_bounds(obj);
}

void ObjSetScale(struct ObjectInstance *obj, double sx, double sy, double sz)
{
	VEC3_SET(obj->scale, sx, sy, sz);
	update_matrix_and_bounds(obj);
}

void ObjSetTransformOrder(struct ObjectInstance *obj, int order)
{
	assert(IsValidTransformOrder(order));
	obj->xform_order = order;
	update_matrix_and_bounds(obj);
}

void ObjSetRotateOrder(struct ObjectInstance *obj, int order)
{
	assert(IsValidRotatedOrder(order));
	obj->rotate_order = order;
	update_matrix_and_bounds(obj);
}

void ObjSetShader(struct ObjectInstance *obj, const struct Shader *shader)
{
	obj->shader = shader;
}

void ObjSetLightList(struct ObjectInstance *obj, const struct Light **lights, int count)
{
	obj->target_lights = lights;
	obj->n_target_lights = count;
}

void ObjSetReflectTarget(struct ObjectInstance *obj, const struct ObjectGroup *grp)
{
	assert(grp != NULL);
	obj->reflection_target = grp;
}

void ObjSetRefractTarget(struct ObjectInstance *obj, const struct ObjectGroup *grp)
{
	assert(grp != NULL);
	obj->refraction_target = grp;
}

const struct ObjectGroup *ObjGetReflectTarget(const struct ObjectInstance *obj)
{
	return obj->reflection_target;
}

const struct ObjectGroup *ObjGetRefractTarget(const struct ObjectInstance *obj)
{
	return obj->refraction_target;
}

const struct Shader *ObjGetShader(const struct ObjectInstance *obj)
{
	return obj->shader;
}

const struct Light **ObjGetLightList(const struct ObjectInstance *obj)
{
	return obj->target_lights;
}

int ObjGetLightCount(const struct ObjectInstance *obj)
{
	return obj->n_target_lights;
}

int ObjIntersect(const struct ObjectInstance *obj, const struct Ray *ray,
		struct LocalGeometry *isect, double *t_hit)
{
	int hit;
	struct Ray ray_in_objspace;

	if (obj->acc == NULL)
		return 0;

	/* transform ray to object space */
	ray_in_objspace = *ray;
	TransformPoint(ray_in_objspace.orig, &obj->world_to_object);
	TransformVector(ray_in_objspace.dir, &obj->world_to_object);

	hit = AccIntersect(obj->acc, &ray_in_objspace, isect, t_hit);
	if (!hit)
		return 0;

	/* transform intersection back to world space */
	TransformPoint(isect->P, &obj->object_to_world);
	TransformVector(isect->N, &obj->object_to_world);
	VEC3_NORMALIZE(isect->N);

	isect->object = obj;

	return 1;
}

/* ObjectGroup interfaces */
struct ObjectGroup *ObjGroupNew(void)
{
	struct ObjectGroup *grp;

	grp = (struct ObjectGroup *) malloc(sizeof(struct ObjectGroup));
	if (grp == NULL)
		return NULL;

	grp->accelerator = AccNew(ACC_BVH);
	if (grp->accelerator == NULL) {
		ObjGroupFree(grp);
		return NULL;
	}

	grp->objects = ArrNew(sizeof(struct ObjectInstance *));
	if (grp->objects == NULL) {
		ObjGroupFree(grp);
		return NULL;
	}

	BOX3_SET(grp->bounds, FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

	return grp;
}

void ObjGroupFree(struct ObjectGroup *grp)
{
	if (grp == NULL)
		return;

	if (grp->accelerator != NULL)
		AccFree(grp->accelerator);

	if (grp->objects != NULL)
		ArrFree(grp->objects);

	free(grp);
}

void ObjGroupAdd(struct ObjectGroup *grp, const struct ObjectInstance *obj)
{
	ArrPushPointer(grp->objects, obj);
	BoxAddBox(grp->bounds, obj->bounds);
	update_group_accelerator(grp);
}

const struct Accelerator *ObjGroupGetAccelerator(const struct ObjectGroup *grp)
{
	return grp->accelerator;
}

static void update_matrix_and_bounds(struct ObjectInstance *obj)
{
	ComputeMatrix(obj->xform_order, obj->rotate_order,
			obj->translate[0], obj->translate[1], obj->translate[2],
			obj->rotate[0],    obj->rotate[1],    obj->rotate[2],
			obj->scale[0],     obj->scale[1],     obj->scale[2],
			&obj->object_to_world);

	MatInverse(&obj->world_to_object, &obj->object_to_world);
	update_object_bounds(obj);
}

static void update_object_bounds(struct ObjectInstance *obj)
{
	AccGetBounds(obj->acc, obj->bounds);
	TransformBounds(obj->bounds, &obj->object_to_world);
}

static void object_bounds(const void *prim_set, int prim_id, double *bounds)
{
	const struct ObjectInstance **objects = (const struct ObjectInstance **) prim_set;
	BOX3_COPY(bounds, objects[prim_id]->bounds);
}

static int object_ray_intersect(const void *prim_set, int prim_id, const struct Ray *ray,
		struct LocalGeometry *isect, double *t_hit)
{
	const struct ObjectInstance **objects = (const struct ObjectInstance **) prim_set;
	return ObjIntersect(objects[prim_id], ray, isect, t_hit);
}

static void update_group_accelerator(struct ObjectGroup *grp)
{
	assert(grp != NULL);
	assert(grp->accelerator != NULL);
	assert(grp->objects != NULL);

	AccSetTargetGeometry(grp->accelerator,
			grp->objects->data,
			grp->objects->nelems,
			grp->bounds,
			object_ray_intersect,
			object_bounds);
}
