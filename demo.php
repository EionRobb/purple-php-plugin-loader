<?php

function init_plugin()
{

}
function plugin_load($plugin = null)
{
	purple_debug(PURPLE_DEBUG_INFO, "demo-php", "Hello from PHP land!\n");
	purple_notify_message($plugin, PURPLE_NOTIFY_MSG_INFO, "Hello!", "Hello from PHP land!", "This is a call from PHP to ph7 to C to libpurple!", null, null);
	
	return true;
}
function plugin_unload()
{
	return true;
}

$info = array(
	'major_version' => 2,
	'minor_version' => 1,
	'type' => 0 /*TODO PURPLE_PLUGIN_LOADER*/,
	'ui_requirement' => NULL,
	'flags' => 0,
	'dependencies' => NULL,
	'priority' => PURPLE_PRIORITY_DEFAULT,

	'id' => "php-demo",
	'name' => "Demo PHP Plugin",
	'version' => '0',
	'summary' => 'Demo\'s a PHP plugin being loaded',
	'description' => "Demo's a PHP plugin being loaded",
	'author' => "Eion Robb <eionrobb@gmail.com>",
	'homepage' => "",

	'load' => plugin_load,
	'unload' => plugin_unload,
	'destroy' => NULL,

	'ui_info' => NULL,
	'extra_info' => NULL,
	'prefs_info' => NULL,
	'actions' => NULL,
);

PURPLE_INIT_PLUGIN('demo', init_plugin, $info)