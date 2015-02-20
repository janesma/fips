#include "publish.h"

#include <stddef.h>

#include "gfcontrol.h"
#include "gfcontrol_stub.h"
#include "gfcpu_freq_control.h"
#include "gfcpu_source.h"
#include "gfgl_source.h"
#include "gfgpu_perf_source.h"
#include "gfpublisher.h"
#include "gfpublisher_skel.h"
#include "gfcpu_clock_source.h"

using Grafips::ControlRouterTarget;
using Grafips::ControlSkel;
using Grafips::CpuFreqControl;
using Grafips::CpuSource;
using Grafips::GlSource;
using Grafips::GpuPerfSource;
using Grafips::PublisherImpl;
using Grafips::PublisherSkeleton;
using Grafips::CpuFreqSource;

CpuSource *prov = NULL;
GlSource *gl_source = NULL;
GpuPerfSource *gpu_source = NULL;
CpuFreqSource *cpu_freq_source = NULL;

void create_publishers()
{
	PublisherImpl *pub = new PublisherImpl;
	prov = new CpuSource;
	gl_source = new GlSource;
	gpu_source = new GpuPerfSource;
	cpu_freq_source = new CpuFreqSource;
	atexit(restore_cpu_freq);

	pub->RegisterSource(prov);
	pub->RegisterSource(gl_source);
	pub->RegisterSource(gpu_source);
	pub->RegisterSource(cpu_freq_source);

	PublisherSkeleton *skel = new PublisherSkeleton(53136, pub);
	skel->Start();

	CpuFreqControl *freq_control = new CpuFreqControl;
	ControlRouterTarget *target = new ControlRouterTarget;
	target->AddControl("CpuFrequencyPolicy", freq_control);
	ControlSkel *control_skel = new ControlSkel(53136 + 1, target);
	control_skel->Start();
}

void publish()
{
	if (prov)
		prov->Poll();
	if (gl_source)
		gl_source->glSwapBuffers();
	if (gpu_source)
		gpu_source->glSwapBuffers();
	if (cpu_freq_source)
		cpu_freq_source->Poll();
}

void grafips_context_init()
{
	if (gpu_source)
		gpu_source->MakeContextCurrent();
}
