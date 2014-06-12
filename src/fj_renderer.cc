/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_renderer.h"
#include "fj_multi_thread.h"
#include "fj_framebuffer.h"
#include "fj_rectangle.h"
#include "fj_progress.h"
#include "fj_property.h"
#include "fj_numeric.h"
#include "fj_sampler.h"
#include "fj_shading.h"
#include "fj_camera.h"
#include "fj_filter.h"
#include "fj_vector.h"
#include "fj_light.h"
#include "fj_tiler.h"
#include "fj_timer.h"
#include "fj_ray.h"
#include "fj_box.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

namespace fj {

class FrameProgress {
public:
  FrameProgress() :
      timer(),
      progress(),
      iteration_list(),
      current_segment(0) {}
  ~FrameProgress() {}

  Timer timer;
  Progress progress;
  Iteration iteration_list[10]; // one for 10% 100% in total
  int current_segment;
};

static Iteration count_total_samples(const struct Tiler *tiler,
    int x_pixel_samples, int y_pixel_samples,
    float x_filter_width, float y_filter_width)
{
  const int tile_count = tiler->GetTileCount();
  int total_sample_count = 0;
  int i;

  for (i = 0; i < tile_count; i++) {
    const Tile *tile = tiler->GetTile(i);
    Rectangle region;
    int samples_in_tile = 0;

    region.xmin = tile->xmin;
    region.ymin = tile->ymin;
    region.xmax = tile->xmax;
    region.ymax = tile->ymax;

    samples_in_tile = SmpGetSampleCountForRegion(&region,
        x_pixel_samples,
        y_pixel_samples,
        x_filter_width,
        y_filter_width);

    total_sample_count += samples_in_tile;
  }
  return total_sample_count;
}

static void distribute_progress_iterations(struct FrameProgress *progress, Iteration total_iteration_count)
{
  const Iteration partial_itr = (Iteration) floor(total_iteration_count / 10.);
  Iteration remains = total_iteration_count - partial_itr * 10;
  int i;

  for (i = 0; i < 10; i++) {
    const Iteration this_itr = partial_itr + (remains > 0 ? 1 : 0);
    progress->iteration_list[i] = this_itr;

    if (remains > 0) {
      remains--;
    }
  }

  progress->current_segment = 0;
}

static void init_frame_progress(struct FrameProgress *progress, const struct Tiler *tiler,
    int x_pixel_samples, int y_pixel_samples,
    float x_filter_width, float y_filter_width)
{
  const Iteration total_iteration_count = count_total_samples(
      tiler,
      x_pixel_samples,
      y_pixel_samples,
      x_filter_width,
      y_filter_width);

  distribute_progress_iterations(progress, total_iteration_count);
}

static Interrupt default_frame_start(void *data, const struct FrameInfo *info)
{
  struct FrameProgress *fp = (struct FrameProgress *) data;

  fp->timer.Start();
  printf("# Rendering Frame\n");
  printf("#   Thread Count: %4d\n", info->worker_count);
  printf("#   Tile Count:   %4d\n", info->tile_count);
  printf("\n");

  {
    int idx;
    fp->current_segment = 0;
    idx = fp->current_segment;
    fp->progress.Start(fp->iteration_list[idx]);
  }

  return CALLBACK_CONTINUE;
}
static Interrupt default_frame_done(void *data, const struct FrameInfo *info)
{
  struct FrameProgress *fp = (struct FrameProgress *) data;
  Elapse elapse;

  elapse = fp->timer.GetElapse();
  printf("# Frame Done\n");
  printf("#   %dh %dm %ds\n", elapse.hour, elapse.min, elapse.sec);
  printf("\n");

  return CALLBACK_CONTINUE;
}

static Interrupt default_tile_start(void *data, const struct TileInfo *info)
{
  return CALLBACK_CONTINUE;
}
static void increment_progress(void *data)
{
  struct FrameProgress *fp = (struct FrameProgress *) data;
  const ProgressStatus status = fp->progress.Increment();

  if (status == PROGRESS_DONE) {
    Elapse elapse;
    int idx;

    fp->current_segment++;
    idx = fp->current_segment;
    elapse = fp->timer.GetElapse();

    printf(" %3d%%  ", idx * 10); 
    printf("(%dh %dm %ds)\n", elapse.hour, elapse.min, elapse.sec);
    fp->progress.Done();

    if (idx != 10) {
      fp->progress.Start(fp->iteration_list[idx]);
    }
  }
}
static Interrupt default_sample_done(void *data)
{
  MtCriticalSection(data, increment_progress);

  return CALLBACK_CONTINUE;
}
static Interrupt default_tile_done(void *data, const struct TileInfo *info)
{
  return CALLBACK_CONTINUE;
}

class Renderer {
public:
  Renderer();
  ~Renderer();

public:
  struct Camera *camera_;
  struct FrameBuffer *framebuffer_;
  struct ObjectGroup *target_objects_;
  struct Light **target_lights_;
  int nlights_;

  int resolution_[2];
  struct Rectangle frame_region_;
  int pixelsamples_[2];
  int tilesize_[2];
  float filterwidth_[2];
  float jitter_;
  double sample_time_start_;
  double sample_time_end_;

  int cast_shadow_;
  int max_reflect_depth_;
  int max_refract_depth_;

  double raymarch_step_;
  double raymarch_shadow_step_;
  double raymarch_reflect_step_;
  double raymarch_refract_step_;

  int use_max_thread_;
  int thread_count_;

  struct FrameReport frame_report_;
  struct TileReport tile_report_;
  struct FrameProgress frame_progress_;
};

Renderer::Renderer()
{
  camera_ = NULL;
  framebuffer_ = NULL;
  target_objects_ = NULL;
  target_lights_ = NULL;
  nlights_ = 0;

  // TODO remove 'this'
  RdrSetResolution(this, 320, 240);
  RdrSetPixelSamples(this, 3, 3);
  RdrSetTileSize(this, 64, 64);
  RdrSetFilterWidth(this, 2, 2);
  RdrSetSampleJitter(this, 1);
  RdrSetSampleTimeRange(this, 0, 1);

  RdrSetShadowEnable(this, 1);
  RdrSetMaxReflectDepth(this, 3);
  RdrSetMaxRefractDepth(this, 3);

  RdrSetRaymarchStep(this, .05);
  RdrSetRaymarchShadowStep(this, .1);
  RdrSetRaymarchReflectStep(this, .1);
  RdrSetRaymarchRefractStep(this, .1);

  RdrSetUseMaxThread(this, 0);
  RdrSetThreadCount(this, 1);

  RdrSetFrameReportCallback(this, &frame_progress_,
      default_frame_start,
      default_frame_done);

  RdrSetTileReportCallback(this, &frame_progress_,
      default_tile_start,
      default_sample_done,
      default_tile_done);
}

Renderer::~Renderer()
{
}

static int prepare_render(struct Renderer *renderer);
static int render_scene(struct Renderer *renderer);

struct Renderer *RdrNew(void)
{
  return new Renderer();
}

void RdrFree(struct Renderer *renderer)
{
  delete renderer;
}

void RdrSetResolution(struct Renderer *renderer, int xres, int yres)
{
  assert(xres > 0);
  assert(yres > 0);
  renderer->resolution_[0] = xres;
  renderer->resolution_[1] = yres;

  RdrSetRenderRegion(renderer, 0, 0, xres, yres);
}

void RdrSetRenderRegion(struct Renderer *renderer, int xmin, int ymin, int xmax, int ymax)
{
  assert(xmin >= 0);
  assert(ymin >= 0);
  assert(xmax >= 0);
  assert(ymax >= 0);
  assert(xmin < xmax);
  assert(ymin < ymax);

  renderer->frame_region_.xmin = xmin;
  renderer->frame_region_.ymin = ymin;
  renderer->frame_region_.xmax = xmax;
  renderer->frame_region_.ymax = ymax;
}

void RdrSetPixelSamples(struct Renderer *renderer, int xrate, int yrate)
{
  assert(xrate > 0);
  assert(yrate > 0);
  renderer->pixelsamples_[0] = xrate;
  renderer->pixelsamples_[1] = yrate;
}

void RdrSetTileSize(struct Renderer *renderer, int xtilesize, int ytilesize)
{
  assert(xtilesize > 0);
  assert(ytilesize > 0);
  renderer->tilesize_[0] = xtilesize;
  renderer->tilesize_[1] = ytilesize;
}

void RdrSetFilterWidth(struct Renderer *renderer, float xfwidth, float yfwidth)
{
  assert(xfwidth > 0);
  assert(yfwidth > 0);
  renderer->filterwidth_[0] = xfwidth;
  renderer->filterwidth_[1] = yfwidth;
}

void RdrSetSampleJitter(struct Renderer *renderer, float jitter)
{
  assert(jitter >= 0 && jitter <= 1);
  renderer->jitter_ = jitter;
}

void RdrSetSampleTimeRange(struct Renderer *renderer, double start_time, double end_time)
{
  assert(start_time <= end_time);

  renderer->sample_time_start_ = start_time;
  renderer->sample_time_end_ = end_time;
}

void RdrSetShadowEnable(struct Renderer *renderer, int enable)
{
  assert(enable == 0 || enable == 1);
  renderer->cast_shadow_ = enable;
}

void RdrSetMaxReflectDepth(struct Renderer *renderer, int max_depth)
{
  assert(max_depth >= 0);
  renderer->max_reflect_depth_ = max_depth;
}

void RdrSetMaxRefractDepth(struct Renderer *renderer, int max_depth)
{
  assert(max_depth >= 0);
  renderer->max_refract_depth_ = max_depth;
}

void RdrSetRaymarchStep(struct Renderer *renderer, double step)
{
  assert(step > 0);
  renderer->raymarch_step_ = Max(step, .001);
}

void RdrSetRaymarchShadowStep(struct Renderer *renderer, double step)
{
  assert(step > 0);
  renderer->raymarch_shadow_step_ = Max(step, .001);
}

void RdrSetRaymarchReflectStep(struct Renderer *renderer, double step)
{
  assert(step > 0);
  renderer->raymarch_reflect_step_ = Max(step, .001);
}

void RdrSetRaymarchRefractStep(struct Renderer *renderer, double step)
{
  assert(step > 0);
  renderer->raymarch_refract_step_ = Max(step, .001);
}

void RdrSetCamera(struct Renderer *renderer, struct Camera *cam)
{
  assert(cam != NULL);
  renderer->camera_ = cam;
}

void RdrSetFrameBuffers(struct Renderer *renderer, struct FrameBuffer *fb)
{
  assert(fb != NULL);
  renderer->framebuffer_ = fb;
}

void RdrSetTargetObjects(struct Renderer *renderer, struct ObjectGroup *grp)
{
  assert(grp != NULL);
  renderer->target_objects_ = grp;
}

void RdrSetTargetLights(struct Renderer *renderer, struct Light **lights, int nlights)
{
  assert(lights != NULL);
  assert(nlights > 0);
  renderer->target_lights_ = lights;
  renderer->nlights_ = nlights;
}

void RdrSetUseMaxThread(struct Renderer *renderer, int use_max_thread)
{
  renderer->use_max_thread_ = (use_max_thread != 0);
}

void RdrSetThreadCount(struct Renderer *renderer, int thread_count)
{
  const int max_thread_count = MtGetMaxThreadCount();

  if (thread_count < 1) {
    renderer->thread_count_ = 1;
  } else if (thread_count > max_thread_count) {
    renderer->thread_count_ = max_thread_count;
  } else {
    renderer->thread_count_ = thread_count;
  }
}

int RdrGetThreadCount(const struct Renderer *renderer)
{
  const int max_thread_count = MtGetMaxThreadCount();

  if (renderer->use_max_thread_) {
    return max_thread_count;
  } else {
    return renderer->thread_count_;
  }
}

int RdrRender(struct Renderer *renderer)
{
  int err = 0;

  err = prepare_render(renderer);
  if (err) {
    /* TODO error handling */
    return -1;
  }

  err = render_scene(renderer);
  if (err) {
    /* TODO error handling */
    return -1;
  }

  return 0;
}

void RdrSetFrameReportCallback(struct Renderer *renderer, void *data,
    FrameStartCallback frame_start,
    FrameDoneCallback frame_done)
{
  CbSetFrameReport(&renderer->frame_report_,
      data,
      frame_start,
      frame_done);
}

void RdrSetTileReportCallback(struct Renderer *renderer, void *data,
    TileStartCallback tile_start,
    SampleDoneCallback sample_done,
    TileDoneCallback tile_done)
{
  CbSetTileReport(&renderer->tile_report_,
      data,
      tile_start,
      sample_done,
      tile_done);
}

static int preprocess_camera(const struct Renderer *renderer)
{
  struct Camera *cam = renderer->camera_;
  int xres, yres;

  if (cam == NULL)
    return -1;

  xres = renderer->resolution_[0];
  yres = renderer->resolution_[1];
  cam->SetAspect(xres/(double)yres);

  return 0;
}

static int preprocess_framebuffer(const struct Renderer *renderer)
{
  struct FrameBuffer *fb = renderer->framebuffer_;
  int xres, yres;

  if (fb == NULL)
    return -1;

  xres = renderer->resolution_[0];
  yres = renderer->resolution_[1];
  fb->Resize(xres, yres, 4);

  return 0;
}

static int preprocess_lights(struct Renderer *renderer)
{
  Timer timer;
  Elapse elapse;
  const int NLIGHTS = renderer->nlights_;
  int i;

  printf("# Preprocessing Lights\n");
  printf("#   Light Count: %d\n", NLIGHTS);
  timer.Start();

  for (i = 0; i < NLIGHTS; i++) {
    struct Light *light = renderer->target_lights_[i];
    const int err = light->Preprocess();

    if (err) {
      /* TODO error handling */
      return -1;
    }
  }

  elapse = timer.GetElapse();
  printf("# Preprocessing Lights Done\n");
  printf("#   %dh %dm %ds\n\n", elapse.hour, elapse.min, elapse.sec);

  return 0;
}

static int prepare_render(struct Renderer *renderer)
{
  int err = 0;

  err = preprocess_camera(renderer);
  if (err) {
    /* TODO error handling */
    return -1;
  }

  err = preprocess_framebuffer(renderer);
  if (err) {
    /* TODO error handling */
    return -1;
  }

  err = preprocess_lights(renderer);
  if (err) {
    /* TODO error handling */
    return -1;
  }

  return 0;
}

struct Worker {
  int id;
  int region_id;
  int region_count;
  int xres, yres;

  const struct Camera *camera;
  struct FrameBuffer *framebuffer;
  struct Sampler *sampler;
  struct Filter *filter;
  struct Sample *pixel_samples;

  struct TraceContext context;
  struct Rectangle tile_region;

  struct TileReport tile_report;

  const struct Tiler *tiler;
};

static void init_worker(struct Worker *worker, int id,
    const struct Renderer *renderer, const struct Tiler *tiler)
{
  const int xres = renderer->resolution_[0];
  const int yres = renderer->resolution_[1];
  const int xrate = renderer->pixelsamples_[0];
  const int yrate = renderer->pixelsamples_[1];
  const double xfwidth = renderer->filterwidth_[0];
  const double yfwidth = renderer->filterwidth_[1];

  worker->camera = renderer->camera_;
  worker->framebuffer = renderer->framebuffer_;
  worker->tiler = tiler;
  worker->id = id;

  worker->region_id = -1;
  worker->region_count = -1;
  worker->xres = xres;
  worker->yres = yres;

  /* Sampler */
  worker->sampler = SmpNew(xres, yres, xrate, yrate, xfwidth, yfwidth);
  if (worker->sampler == NULL) {
    /*
    render_state = -1;
    goto cleanup_and_exit;
    */
  }
  SmpSetJitter(worker->sampler, renderer->jitter_);
  SmpSetSampleTimeRange(worker->sampler,
      renderer->sample_time_start_, renderer->sample_time_end_);
  worker->pixel_samples = SmpAllocatePixelSamples(worker->sampler);

  /* Filter */
  worker->filter = FltNew(FLT_GAUSSIAN, xfwidth, yfwidth);
  if (worker->filter == NULL) {
    /*
    render_state = -1;
    goto cleanup_and_exit;
    */
  }

  /* context */
  worker->context = SlCameraContext(renderer->target_objects_);
  worker->context.cast_shadow = renderer->cast_shadow_;
  worker->context.max_reflect_depth = renderer->max_reflect_depth_;
  worker->context.max_refract_depth = renderer->max_refract_depth_;
  worker->context.raymarch_step = renderer->raymarch_step_;
  worker->context.raymarch_shadow_step = renderer->raymarch_shadow_step_;
  worker->context.raymarch_reflect_step = renderer->raymarch_reflect_step_;
  worker->context.raymarch_refract_step = renderer->raymarch_refract_step_;

  /* region */
  worker->tile_region.xmin = 0;
  worker->tile_region.xmax = 0;
  worker->tile_region.ymin = 0;
  worker->tile_region.ymax = 0;

  /* interruption */
  worker->tile_report = renderer->tile_report_;
}

static struct Worker *new_worker_list(int worker_count,
    const struct Renderer *renderer, const struct Tiler *tiler)
{
  Worker *worker_list = new Worker[worker_count];
  int i;

  if (worker_list == NULL) {
    return NULL;
  }

  for (i = 0; i < worker_count; i++) {
    init_worker(&worker_list[i], i, renderer, tiler);
  }

  return worker_list;
}

static void set_working_region(struct Worker *worker, int region_id)
{
  const Tile *tile = worker->tiler->GetTile(region_id);

  worker->region_id = region_id;
  worker->region_count = worker->tiler->GetTileCount();
  worker->tile_region.xmin = tile->xmin;
  worker->tile_region.ymin = tile->ymin;
  worker->tile_region.xmax = tile->xmax;
  worker->tile_region.ymax = tile->ymax;

  if (SmpGenerateSamples(worker->sampler, &worker->tile_region)) {
    /* TODO error handling */
  }
}

static void finish_worker(struct Worker *worker)
{
  SmpFreePixelSamples(worker->pixel_samples);
  SmpFree(worker->sampler);
  FltFree(worker->filter);
}

static void free_worker_list(struct Worker *worker_list, int worker_count)
{
  int i;

  if (worker_list == NULL) {
    return;
  }

  for (i = 0; i < worker_count; i++) {
    finish_worker(&worker_list[i]);
  }

  delete [] worker_list;
}

static struct Color4 apply_pixel_filter(struct Worker *worker, int x, int y)
{
  const int nsamples = SmpGetSampleCountForPixel(worker->sampler);
  const int xres = worker->xres;
  const int yres = worker->yres;
  struct Sample *pixel_samples = worker->pixel_samples;
  struct Filter *filter = worker->filter;

  struct Color4 pixel;
  float wgt_sum = 0.f;
  float inv_sum = 0.f;
  int i;

  for (i = 0; i < nsamples; i++) {
    struct Sample *sample = &pixel_samples[i];
    double filtx = 0, filty = 0;
    double wgt = 0;

    filtx = xres * sample->uv.x - (x + .5);
    filty = yres * (1-sample->uv.y) - (y + .5);
    wgt = filter->Evaluate(filtx, filty);

    pixel.r += wgt * sample->data[0];
    pixel.g += wgt * sample->data[1];
    pixel.b += wgt * sample->data[2];
    pixel.a += wgt * sample->data[3];
    wgt_sum += wgt;
  }

  inv_sum = 1.f / wgt_sum;
  pixel.r *= inv_sum;
  pixel.g *= inv_sum;
  pixel.b *= inv_sum;
  pixel.a *= inv_sum;

  return pixel;
}

static void reconstruct_image(struct Worker *worker)
{
  struct FrameBuffer *fb = worker->framebuffer;
  const int xmin = worker->tile_region.xmin;
  const int ymin = worker->tile_region.ymin;
  const int xmax = worker->tile_region.xmax;
  const int ymax = worker->tile_region.ymax;
  int x, y;

  for (y = ymin; y < ymax; y++) {
    for (x = xmin; x < xmax; x++) {
      struct Color4 pixel;

      SmpGetPixelSamples(worker->sampler, worker->pixel_samples, x, y);
      pixel = apply_pixel_filter(worker, x, y);

      fb->SetColor(x, y, pixel);
    }
  }
}

static void render_frame_start(struct Renderer *renderer, const struct Tiler *tiler)
{
  struct FrameInfo info;
  info.worker_count = RdrGetThreadCount(renderer);
  info.tile_count = tiler->GetTileCount();
  info.frame_region = renderer->frame_region_;;
  info.framebuffer = renderer->framebuffer_;

  CbReportFrameStart(&renderer->frame_report_, &info);
}

static void render_frame_done(struct Renderer *renderer, const struct Tiler *tiler)
{
  struct FrameInfo info;
  info.worker_count = RdrGetThreadCount(renderer);
  info.tile_count = tiler->GetTileCount();
  info.frame_region = renderer->frame_region_;;
  info.framebuffer = renderer->framebuffer_;

  CbReportFrameDone(&renderer->frame_report_, &info);
}

static void render_tile_start(struct Worker *worker)
{
  struct TileInfo info;
  info.worker_id = worker->id;
  info.region_id = worker->region_id;
  info.total_region_count = worker->region_count;
  info.total_sample_count = SmpGetSampleCount(worker->sampler);
  info.tile_region = worker->tile_region;
  info.framebuffer = worker->framebuffer;

  CbReportTileStart(&worker->tile_report, &info);
}

static void render_tile_done(struct Worker *worker)
{
  struct TileInfo info;
  info.worker_id = worker->id;
  info.region_id = worker->region_id;
  info.total_region_count = worker->region_count;
  info.total_sample_count = SmpGetSampleCount(worker->sampler);
  info.tile_region = worker->tile_region;
  info.framebuffer = worker->framebuffer;

  CbReportTileDone(&worker->tile_report, &info);
}

static int integrate_samples(struct Worker *worker)
{
  struct Sample *smp = NULL;
  struct TraceContext cxt = worker->context;
  struct Ray ray;

  while ((smp = SmpGetNextSample(worker->sampler)) != NULL) {
    struct Color4 C_trace;
    double t_hit = FLT_MAX;
    int hit = 0;
    int interrupted = 0;

    worker->camera->GetRay(smp->uv, smp->time, &ray);
    cxt.time = smp->time;

    hit = SlTrace(&cxt, &ray.orig, &ray.dir, ray.tmin, ray.tmax, &C_trace, &t_hit);
    if (hit) {
      smp->data[0] = C_trace.r;
      smp->data[1] = C_trace.g;
      smp->data[2] = C_trace.b;
      smp->data[3] = C_trace.a;
    } else {
      smp->data[0] = 0;
      smp->data[1] = 0;
      smp->data[2] = 0;
      smp->data[3] = 0;
    }

    interrupted = CbReportSampleDone(&worker->tile_report);
    if (interrupted) {
      printf("integrate_samples CANCELED!\n");
      return -1;
    }
  }
  return 0;
}

static ThreadStatus render_tile(void *data, const struct ThreadContext *context)
{
  struct Worker *worker_list = (struct Worker *) data;
  struct Worker *worker = &worker_list[context->thread_id];
  int interrupted = 0;

  set_working_region(worker, context->iteration_id);

  render_tile_start(worker);

  interrupted = integrate_samples(worker);
  reconstruct_image(worker);

  render_tile_done(worker);

  if (interrupted) {
    return THREAD_LOOP_CANCEL;
  }

  return THREAD_LOOP_CONTINUE;
}

static int render_scene(struct Renderer *renderer)
{
  struct Worker *worker_list = NULL;
  struct Tiler *tiler = NULL;

  const int thread_count = RdrGetThreadCount(renderer);
  int render_state = 0;
  int tile_count = 0;

  const int xres = renderer->resolution_[0];
  const int yres = renderer->resolution_[1];
  const int xtilesize = renderer->tilesize_[0];
  const int ytilesize = renderer->tilesize_[1];

  const int xpixelsamples = renderer->pixelsamples_[0];
  const int ypixelsamples = renderer->pixelsamples_[1];
  const float xfilterwidth = renderer->filterwidth_[0];
  const float yfilterwidth = renderer->filterwidth_[1];

  /* Tiler */
  tiler = TlrNew(xres, yres, xtilesize, ytilesize);
  if (tiler == NULL) {
    render_state = -1;
    goto cleanup_and_exit;
  }
  tiler->GenerateTiles(renderer->frame_region_);
  tile_count = tiler->GetTileCount();

  /* Worker */
  worker_list = new_worker_list(thread_count, renderer, tiler);
  if (worker_list == NULL) {
    render_state = -1;
    goto cleanup_and_exit;
  }

  /* FrameProgress */
  init_frame_progress(&renderer->frame_progress_, tiler,
      xpixelsamples, ypixelsamples,
      xfilterwidth, yfilterwidth);

  /* Run sampling */
  render_frame_start(renderer, tiler);

  MtRunThreadLoop(worker_list, render_tile, thread_count, 0, tile_count);

  render_frame_done(renderer, tiler);

cleanup_and_exit:
  TlrFree(tiler);
  free_worker_list(worker_list, thread_count);

  return render_state;
}

} // namespace xxx
