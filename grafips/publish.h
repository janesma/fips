#ifndef _FIPS_PUBLISH_H__
#define _FIPS_PUBLISH_H__

#include "fips-dispatch-gl.h"

#ifdef __cplusplus
extern "C" {
#endif
	void create_publishers();
	void publish();
	void grafips_context_init();
	bool perform_draw_experiments();
	void perform_bind_texture_experiment(GLenum target);
	void publish_context(void *context);
	void on_link_program(GLint program);
	void on_use_program(GLint program);
#ifdef __cplusplus
}
#endif

#endif
