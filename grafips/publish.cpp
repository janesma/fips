#include "publish.h"

#include <stddef.h>

#include "gfcpu_source.h"
#include "gfgl_source.h"
#include "gfgpu_perf_source.h"
#include "gfpublisher_skel.h"
#include "gfpublisher.h"

using Grafips::CpuSource;
using Grafips::GlSource;
using Grafips::GpuPerfSource;
using Grafips::PublisherImpl;
using Grafips::PublisherSkeleton;

CpuSource *prov = NULL;
GlSource *gl_source = NULL;
GpuPerfSource *gpu_source = NULL;

void create_publishers()
{
	PublisherImpl *pub = new PublisherImpl;
	prov = new CpuSource;
	gl_source = new GlSource;
	gpu_source = new GpuPerfSource;
	pub->RegisterSource(prov);
	pub->RegisterSource(gl_source);
	pub->RegisterSource(gpu_source);

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

void grafips_context_init()
{
	if (gpu_source)
		gpu_source->MakeContextCurrent();
}
