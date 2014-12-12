#include "publish.h"

#include <stddef.h>

#include "gfcpu_source.h"
#include "gfgl_source.h"
#include "gfpublisher_skel.h"
#include "gfpublisher.h"

using Grafips::CpuSource;
using Grafips::GlSource;
using Grafips::PublisherImpl;
using Grafips::PublisherSkeleton;

CpuSource *prov = NULL;
GlSource *gl_source = NULL;
void create_publishers()
{
  prov = new CpuSource;
  PublisherImpl *pub = new PublisherImpl;

  prov->SetMetricSink(pub);
  gl_source = new GlSource(pub);

  PublisherSkeleton *skel = new PublisherSkeleton(53136, pub);
  skel->Start();
}

void publish()
{
  if (prov)
    prov->Poll();
  if (gl_source)
    gl_source->glSwapBuffers();
}
