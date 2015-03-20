#include "publish.h"

#include <stddef.h>
#include <stdio.h>

#include "gfapi_control.h"
#include "gfcontrol.h"
#include "gfcontrol_stub.h"
#include "gfcpu_clock_source.h"
#include "gfcpu_freq_control.h"
#include "gfcpu_source.h"
#include "gferror.h"
#include "gfgl_source.h"
#include "gfgpu_perf_functions.h"
#include "gfgpu_perf_source.h"
#include "gflog.h"
#include "gfpublisher.h"
#include "gfpublisher_skel.h"
#include "glwrap.h"

using Grafips::ApiControl;
using Grafips::ControlRouterTarget;
using Grafips::ControlSkel;
using Grafips::CpuFreqControl;
using Grafips::CpuFreqSource;
using Grafips::CpuSource;
using Grafips::ErrorHandler;
using Grafips::ErrorInterface;
using Grafips::GlSource;
using Grafips::GpuPerfSource;
using Grafips::NoError;
using Grafips::PerfFunctions;
using Grafips::PublisherImpl;
using Grafips::PublisherSkeleton;
using Grafips::kSocketReadFail;
using Grafips::kSocketWriteFail;


class GrafipsPublishers {
public:
	GrafipsPublishers() {
		printf("publishers construct\n");
		void *get_proc = fips_lookup("glXGetProcAddress");
		PerfFunctions::Init(get_proc);
		m_prov = new CpuSource;
		m_gl_source = new GlSource(100);
		m_gpu_source = new GpuPerfSource;
		m_cpu_freq_source = new CpuFreqSource;

		m_pub = new PublisherImpl;
		m_pub->RegisterSource(m_prov);
		m_pub->RegisterSource(m_gl_source);
		m_pub->RegisterSource(m_gpu_source);
		m_pub->RegisterSource(m_cpu_freq_source);

		int port = 53136;  // default port
		const char *env_port = getenv("FIPS_PORT");
		if (env_port != NULL)
			port = atoi(env_port);
		m_skel = new PublisherSkeleton(port, m_pub);
		m_skel->Start();

		m_freq_control = new CpuFreqControl;
		m_api_control = new ApiControl;
		m_target = new ControlRouterTarget;
		m_target->AddControl("CpuFrequencyPolicy", m_freq_control);
		m_target->AddControl("ScissorExperiment", m_api_control);
		m_target->AddControl("2x2TextureExperiment", m_api_control);
		m_target->AddControl("SimpleShaderExperiment", m_api_control);
		m_target->AddControl("DisableDrawExperiment", m_api_control);
		m_target->AddControl("WireframeExperiment", m_api_control);
		m_control_skel = new ControlSkel(port + 1, m_target);
		m_control_skel->Start();
	}
	~GrafipsPublishers() {
		printf("publishers destroy\n");
		m_control_skel->Join();
		delete m_control_skel;
		delete m_target;
		delete m_freq_control;
		delete m_api_control;
		
		m_skel->Join();
		delete m_skel;

		delete m_pub;
		delete m_cpu_freq_source;
		delete m_gpu_source;
		delete m_gl_source;
		delete m_prov;
	}
	void MakeContextCurrent() {
		m_gpu_source->MakeContextCurrent();
	}
	bool PerformExperiments() {
		return m_api_control->PerformDrawExperiments();
	}
	void OnLinkProgram(GLint program) {
		m_api_control->OnLinkProgram(program);
	}
	void OnUseProgram(GLint program) {
		m_api_control->OnUseProgram(program);
	}

	void OnContext(void *context) {
		m_api_control->OnContext(context);
	}

	void Publish() {
		if (NoError())
			m_prov->Poll();
		if (NoError())
			m_gl_source->glSwapBuffers();
		if (NoError())
			m_gpu_source->glSwapBuffers();
		if (NoError())
			m_cpu_freq_source->Poll();
	}
private:
	PublisherImpl *m_pub;
	CpuSource *m_prov;
	GlSource *m_gl_source;
	GpuPerfSource *m_gpu_source;
	CpuFreqSource *m_cpu_freq_source;
	PublisherSkeleton *m_skel;
	CpuFreqControl *m_freq_control;
	ApiControl *m_api_control;
	ControlRouterTarget *m_target;
	ControlSkel *m_control_skel;
};


GrafipsPublishers *publishers = NULL;

class DetectClosedHost : public ErrorHandler {
  public:
	bool OnError(const ErrorInterface &e) {
		if ((e.Type() == kSocketWriteFail) || 
		    (e.Type() == kSocketReadFail)) {
			printf("ERROR: %s\n", e.ToString());
			return true;
		    }
		return false;
	}
};

void create_publishers()
{
	publishers = new GrafipsPublishers;
}

void publish()
{
	if (!publishers)
		return;

	DetectClosedHost handler;
	publishers->Publish();
	if (!NoError()) {
		printf("deleting publishers\n");
		delete publishers;
		publishers = NULL;
	}
}

void grafips_context_init()
{
	if (publishers)
		publishers->MakeContextCurrent();
}

bool perform_draw_experiments()
{
	if (publishers)
		return publishers->PerformExperiments();
	return true;
}

void on_link_program(GLint program)
{
	if (publishers)
		publishers->OnLinkProgram(program);
}
void on_use_program(GLint program)
{
	if(publishers)
		publishers->OnUseProgram(program);
}

void publish_context(void* context)
{
	if(publishers)
	 	publishers->OnContext(context);
}
