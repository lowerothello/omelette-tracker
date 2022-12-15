/*
 * omelette's instrument api
 * inspired by ladspa
 */

#define OMUINST_VERSION_MAJOR 0
#define OMUINST_VERSION_MINOR 1


/*
 * omuinst_descriptor is the instrument entry point,
 * and should be present in the plugin's shared object.
 */
const omuinstDescriptor *omuinst_descriptor(void);
