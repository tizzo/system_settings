<?php
// $Id$

/**
 * @file
 *   Provide a way to use swappable plugins for any part of the system as inspired by Crell
 *   (Larry Garfield)'s post here: http://www.garfieldtech.com/blog/drupal-handler-rfc
 *   and here: http://www.garfieldtech.com/blog/handlers-rfc-2
 *
 * If a pluggable handler's system lands in core (fingers crossed) this code will be deprecated.
 */

/**
 * Returns a loaded system_settings handler appropriate to the targer.
 *
 * @param $target
 *   The name of the system_settings target.
 * @return
 *   The appropriate system_settings handler object.
 */
function system_settings_get_target_handler($target) {
  global $system_settings;
  static $handlers = array();
  if (!isset($handlers[$target])) {
    $map = system_settings_handler_map();
    $handler = system_settings_target_handler_factory($map[$target], $system_settings);
  }
  else {
    // If we're using a cached handler make sure its system
    // settings are up to date.
    $conf->system_settings = $system_settings;
    $handler->configure($conf);
  }
  return $handler;
}

/**
 * Load and configure a system_settings handler.
 *
 * @param $handler_data
 *   An array with keys 'handler', 'path', and 'class'
 *   of the handler, path to, and name of the class.
 */
function system_settings_target_handler_factory($handler_data, $system_settings) {
  $path = drupal_get_path('module', 'system_settings') . '/handlers/system_settings.handler.interface.inc';
  require_once($handler_data['path']);
  $handler = 'SystemSettings' . $handler_data['class'] . 'Handler';
  $handler = new $handler;
  $conf = new stdClass;
  $conf->system_settings = $system_settings;
  $handler->configure($conf);
  return $handler;
}

/**
 * Retrieves an array of all system_settings 
 * handlers and the relevant data.
 *
 * @return (array)
 *   An array of all avaialable system_settings handlers.
 */
function system_settings_get_handlers() {
  static $handlers;
  if (is_null($handlers)) {
    $handlers = module_invoke_all('system_settings_handlers');
  }
  return $handlers;
}

/**
 * Retrieves an array of all system_settings 
 * targets.
 *
 * @return (array)
 *   An array of all avaialable system_settings targets.
 */
function system_settings_get_targets() {
  static $targets;
  if (is_null($targets)) {
    $targets = module_invoke_all('system_settings_targets');
  }
  return $targets;
}

/**
 * Implementation of hook_system_settings_handlers().
 */
function system_settings_system_settings_handlers() {
  $path = drupal_get_path('module', 'system_settings') . '/handlers/';
  $handlers = array(
    'default' => array(
      'path' => $path . 'system_settings.handler.default.inc',
      'supported targets' => system_settings_system_settings_targets(),
      'class' => 'Default',
    ),
    'static_files' => array(
      'path' => $path . 'system_settings.handler.static_files.inc',
      'supported targets' => system_settings_system_settings_targets(),
      'class' => 'Default',
    ),
  );
  return $handlers;
}

/**
 * Implementation of hook_system_settings_targets().
 */
function system_settings_system_settings_targets() {
  $targets = array(
    'system_settings',
    'system_settings_log',
  );
  return $targets;
}

/**
 * Get all of the settings defined in code that are available
 * before we can connect to the database. 
 */
function system_settings_pre_init_settings_get($name) {
  global $system_settings;
  $system_settings = system_settings_get_default_settings($system_settings);
  system_settings_load_settings_php_settings($system_settings);
  if (isset($system_settings['settings'][$name])) {
    return $system_settings['settings'][$name];
  }
  else {
    return FALSE;
  }
}

/**
 * Maps system_settings targets to system_settings handlers.
 *
 * @return
 *   An array keyed by target containing arrays with keys for 'handler' and 'path'.
 */
function system_settings_handler_map() {
  static $handler_map;
  if (is_null($handler_map)) {
    $handlers = system_settings_get_handlers(); 
    $handlers_settings = system_settings_pre_init_settings_get('system_settings_handlers');
    $targets = system_settings_get_targets();
    $handler_map = array();
    foreach ($targets as $target) {
      $target_handler = $handlers_settings[$target];
      if (isset($handlers[$target_handler]) && in_array($target, $handlers[$target_handler]['supported targets'])) {
        $handler_map[$target] = array(
          'handler' => $handlers_settings[$target],
          'path' => $handlers[$target_handler]['path'],
          'class' => $handlers[$target_handler]['class'],
        );
      }
    }
  }
  return $handler_map;
}
