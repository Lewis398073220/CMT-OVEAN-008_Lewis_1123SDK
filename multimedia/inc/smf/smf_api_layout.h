#pragma once
#include <smf_common.h>
#ifndef EXTERNC
#ifndef __cplusplus
#define EXTERNC
#else
#define EXTERNC extern "C"
#endif
#endif
typedef struct {
	uint8_t index;
	uint8_t parent;
	uint8_t links[6];
	const char* name;
	//const char* types;//type-keys-subkeys0-subkeys1
	struct {
		const char* type;
		const char* keys;
		const char* subkeys0;
		const char* subkeys1;
	};
	struct proprety_t {
		const char* keys;
		uint32_t vals;
	}props[12];
}smf_layout_t;

/** create a object from layout script.
 * @param script[in] pipeline script list, end for index=0.
 * @param parent[in] set the parent for pipeline.
 * @return return the object pointer.
 */
EXTERNC void* smf_create_object(smf_layout_t* script, void* parent);

/** register pipeline
 */
EXTERNC void smf_pipeline_register();

EXTERNC bool smf_test_layout(const smf_layout_t* serial);
