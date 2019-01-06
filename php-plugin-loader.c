
#include "purple.h"

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include "ph7.c"

typedef struct {
	ph7_value *load_func;
	ph7_value *unload_func;
	ph7_vm *pVm;
} PurplePhpScript;

//static int PH7_VmCallUserFunction(ph7_vm *pVm, ph7_value *pFunc, int nArg, ph7_value **apArg, ph7_value *pResult);
static int PH7_VmCallUserFunctionAp(ph7_vm *pVm, ph7_value *pFunc, ph7_value *pResult, ...);

static ph7 *pEngine = NULL;

static int
ph7ErrorConsumer(const void *pErrMsg, unsigned int msg_len, void *pUserData)
{
	purple_debug_error("php", "%*s\n", msg_len, (const char *)pErrMsg);
	
	return PH7_OK;
}

/*static ph7_value *
php_call_func(ph7_vm *pVm, const gchar *func_name, ...)
{
	ph7_value **apArg;
	ph7_value *pArg;
	ph7_value *pResult;
	ph7_value *pFunc;
	va_list ap;
	int i, len;
	
	va_start(ap, func_name);
	for(len = 0; ; len++){
		pArg = va_arg(ap, ph7_value *);
		if( pArg == 0 ){
			break;
		}
	}
	va_end(ap);
	
	apArg = g_new0(ph7_value *, len + 1);
	
	va_start(ap, func_name);
	for(i = 0; i < len; i++){
		pArg = va_arg(ap, ph7_value *);
		if( pArg == 0 ){
			break;
		}
		apArg[i] = pArg;
	}
	va_end(ap);
	
	
	pFunc = ph7_new_scalar(pVm);
	ph7_value_string(pFunc, func_name, -1);
	pResult = ph7_new_scalar(pVm);
	PH7_VmCallUserFunction(pVm, pFunc, len, apArg, pResult);
	
	ph7_release_value(pVm, pFunc);
	g_free(apArg);
	return pResult;
}*/

static gboolean
plugin_load(PurplePlugin *plugin)
{
	ph7_init(&pEngine);
	ph7_config(pEngine, PH7_CONFIG_ERR_OUTPUT, ph7ErrorConsumer, plugin);
	
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	ph7_release(pEngine);
	pEngine = NULL;
	
	return TRUE;
}

int php__INIT_PLUGIN__call(ph7_context *pCtx, int argc, ph7_value **argv)
{
	PurplePlugin *plugin = ph7_context_user_data(pCtx);
	PurplePhpScript *pps;
	PurplePluginInfo *info;
	ph7_vm *pVm = plugin->handle;
	ph7_value *plugin_info;
	ph7_value *init_func;
	
	if (argc != 3) {
		ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Unexpected number of arguments");
		return PH7_ABORT;
	}
	
	init_func = argv[1];
	plugin_info = argv[2];
	
	if (ph7_value_is_callable(init_func)) {
		PH7_VmCallUserFunctionAp(pVm, init_func, NULL, NULL);
	}

	info = g_new0(PurplePluginInfo, 1);
	pps = g_new0(PurplePhpScript, 1);
	pps->pVm = pVm;

	info->magic = PURPLE_PLUGIN_MAGIC;
	info->major_version = 2;
	info->minor_version = 1;
	info->type = PURPLE_PLUGIN_STANDARD;

	info->dependencies = g_list_append(info->dependencies, "core-eionrobb-php");

	if (ph7_value_is_array(plugin_info)) {
		info->name = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "name", -1), NULL));
		info->id = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "id", -1), NULL));
		info->homepage = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "homepage", -1), NULL));
		info->author = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "author", -1), NULL));
		info->summary = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "summary", -1), NULL));
		info->description = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "description", -1), NULL));
		info->version = g_strdup(ph7_value_to_string(ph7_array_fetch(plugin_info, "version", -1), NULL));
		
		pps->load_func = ph7_array_fetch(plugin_info, "load", -1);
		pps->unload_func = ph7_array_fetch(plugin_info, "unload", -1);
	}

	plugin->info = info;
	plugin->info->extra_info = pps;

	purple_plugin_register(plugin);
	
	return PH7_OK;
}

static gboolean
probe_php_plugin(PurplePlugin *plugin)
{
	ph7_vm *pVm;
	ph7_compile_file(pEngine, plugin->path, &pVm, 0);
	
	plugin->handle = pVm;
	// TODO Add functions to the vm here
	ph7_create_function(pVm, "PURPLE_INIT_PLUGIN", php__INIT_PLUGIN__call, plugin);
	
	ph7_vm_exec(pVm, NULL);
	
	// The plugin gets registered in the PURPLE INIT PLUGIN callback
	return TRUE;
}

static gboolean
load_php_plugin(PurplePlugin *plugin)
{
	PurplePhpScript *pps = (PurplePhpScript *)plugin->info->extra_info;
	ph7_vm *pVm = pps->pVm;
	ph7_value *result = ph7_new_scalar(pVm);
	int ret;

	if (pVm == NULL)
		return FALSE;

	purple_debug(PURPLE_DEBUG_INFO, "php", "Loading PHP script\n");

	ret = PH7_VmCallUserFunctionAp(pVm, pps->load_func, result, NULL); //TODO pass through plugin as an object
	
	if (ret != PH7_OK) {
		return FALSE;
	}
	ret = ph7_value_to_bool(result);

	return ret ? TRUE : FALSE;
}

static gboolean
unload_php_plugin(PurplePlugin *plugin)
{
	PurplePhpScript *pps = (PurplePhpScript *)plugin->info->extra_info;
	ph7_vm *pVm = pps->pVm;
	ph7_value *result = ph7_new_scalar(pVm);
	int ret;

	if (pVm == NULL)
		return FALSE;

	purple_debug(PURPLE_DEBUG_INFO, "php", "Unloading PHP script\n");

	ret = PH7_VmCallUserFunctionAp(pVm, pps->unload_func, result, NULL); //TODO pass through plugin as an object
	
	if (ret != PH7_OK) {
		return FALSE;
	}
	ret = ph7_value_to_bool(result);
	
	return ret ? TRUE : FALSE;
}

static void
destroy_php_plugin(PurplePlugin *plugin)
{
	if (plugin->info != NULL) {
		PurplePhpScript *pps = (PurplePhpScript *)plugin->info->extra_info;
		ph7_vm *pVm;

		g_free(plugin->info->name);
		g_free(plugin->info->id);
		g_free(plugin->info->homepage);
		g_free(plugin->info->author);
		g_free(plugin->info->summary);
		g_free(plugin->info->description);
		g_free(plugin->info->version);

		g_free(pps);
		plugin->info->extra_info = NULL;
		
		pVm = (ph7_vm *)plugin->handle;
		if (pVm != NULL) {
			ph7_vm_release(pVm);
			plugin->handle = NULL;
		}

		g_free(plugin->info);
		plugin->info = NULL;
	}
}

static PurplePluginLoaderInfo loader_info =
{
	NULL,                                            /**< exts           */
	probe_php_plugin,                                /**< probe          */
	load_php_plugin,                                 /**< load           */
	unload_php_plugin,                               /**< unload         */
	destroy_php_plugin,                              /**< destroy        */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	2,
	1,
	PURPLE_PLUGIN_LOADER,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */

	"core-eionrobb-php",                              /**< id             */
	"PH7 Plugin Loader",                              /**< name           */
	"0.1",                                            /**< version        */
	"Provides support for loading PHP plugins.",      /**< summary        */
	"Provides support for loading PHP plugins.",      /**< description    */
	"Eion Robb <eionrobb@gmail.com>",                 /**< author         */
	"",                                               /**< homepage       */

	plugin_load,                                      /**< load           */
	plugin_unload,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&loader_info,                                     /**< extra_info     */
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	loader_info.exts = g_list_append(loader_info.exts, "php");
}

PURPLE_INIT_PLUGIN(php, init_plugin, info)
