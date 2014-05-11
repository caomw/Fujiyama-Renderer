/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#ifndef FJ_POINTCLOUDIO_H
#define FJ_POINTCLOUDIO_H

namespace fj {

struct Vector;

/* point cloud input */
struct PtcInputFile;
extern struct PtcInputFile *PtcOpenInputFile(const char *filename);
extern void PtcCloseInputFile(struct PtcInputFile *in);

extern void PtcReadHeader(struct PtcInputFile *in);
extern void PtcReadData(struct PtcInputFile *in);

extern int PtcGetInputPointCount(const struct PtcInputFile *in);
extern void PtcSetInputPosition(struct PtcInputFile *in, struct Vector *P);
extern void PtcSetInputAttributeDouble(struct PtcInputFile *in,
    const char *attr_name, double *attr_data);
extern void PtcSetInputAttributeVector3(struct PtcInputFile *in,
    const char *attr_name, struct Vector *attr_data);

extern int PtcGetInputAttributeCount(const struct PtcInputFile *in);

/* point cloud output */
struct PtcOutputFile;
extern struct PtcOutputFile *PtcOpenOutputFile(const char *filename);
extern void PtcCloseOutputFile(struct PtcOutputFile *out);

extern void PtcSetOutputPosition(struct PtcOutputFile *out, 
    const struct Vector *P, int point_count);

extern void PtcSetOutputAttributeDouble(struct PtcOutputFile *out, 
    const char *attr_name, const double *attr_data);
extern void PtcSetOutputAttributeVector3(struct PtcOutputFile *out, 
    const char *attr_name, const struct Vector *attr_data);

extern void PtcWriteFile(struct PtcOutputFile *out);


/* high level interface for loading mesh file */
struct PointCloud;
extern int PtcLoadFile(struct PointCloud *ptc, const char *filename);

} // namespace xxx

#endif /* FJ_XXX_H */
