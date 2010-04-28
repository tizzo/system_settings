<?php
// $Id$

/**
 * @file
 *   Defines the default handler system_settings storage backends.
 */

/**
 *
 * TODO: Implement the settings inteface once the API is locked.
 */
class SystemSettingsDefaultHandler /*implements SystemSettingsInterface*/ {

  // Internal copy of the global $system_settings
  protected  $system_settings;

  /**
   * Configure this object using a config object.
   *
   * @param $config (mixed)
   *   An object or array with keys for the    
   */
  public function configure($config) {
    foreach ($config as $property => $value) {
      $this->$property = $value;
    }
  }

  /**
   * Get settings from the storage mechanism.
   *
   * @param $system_settings (array)
   *   A system_settings array.
   *   
   * @return (array)
   *   An updated system_settings array.
   */
  public function getSettings(&$system_settings, $active = FALSE) {
    $result = db_query("SELECT name, value FROM {system_settings} WHERE always_load=%d", EVERY_PAGE);
    while ($setting = db_fetch_object($result)) {
      $setting_array = array();
      $setting_array['value'] = unserialize($setting->value);
      // TODO: get this data
      $setting_array['modules'] = array();
      $setting_array['groups'] = array();
      $setting_array['entities'] = array();
      _system_settings_process_setting_array($setting->name, $setting_array, $system_settings) ;
    }
  }

  /**
   *
   */
  public function getGroups() {
    static $groups = FALSE;
    if (!$groups) {
      $groups = array();
      while ($group = db_result(db_query("SELECT settings_group FROM {system_settings_group}"))) {
        $groups[] = $group;
      }
    }
    return $groups;
  }

  /**
   * @param (string) $group
   *   The name of the group to load settings for.
   * @return (array)
   *   An array of the settings relevant to this group.
   */
  public function getGroupSettings($group, $active = FALSE) {
    if (!$active && isset($system_settings['groups'][$group])) {
      return $system_settings['groups'][$group];
    }
    $result = db_query("SELECT ss.name, ss.value FROM {system_settings} as ss JOIN {system_settings_group} AS ssg ON ss.name = ssg.name WHERE ssg.group = %s", $group);
    $settings = array();
    while ($setting = db_fetch_object($result)) {
      $settings[$setting->name] = $setting->value;
    }
    return $settings;
  }

  /**
   * Cleanup orphaned settings that are no longer 
   * associated with any module or theme.
   */
  public function removeOrphanSettings() {
    return FALSE;
  }

  /**
   *
   */
  public function getSetting($name) {
    return unserialize(db_result(db_query("SELECT value FROM {system_settings} WHERE package = '%s' AND name = '%s'"), $package, $name));
  }

  /**
   * 
   */
  public function storeSetting($setting_array) {
    if (!db_fetch_result(db_query("SELECT name FROM {system_settings} WHERE name='%s'", $name))) {
      db_query("INSERT INTO {system_settings} (package, name, value, load_frequency) VALUES ('%s', '%s', '%s')", $package, $name, serialize($value));
    }
    else {
      db_query("UPDATE {system_settings} SET value = %s, load_frequency = %s WHERE package = '%s' AND name = '%s'", serialize($value), $load_frequency, $package, $name);
    }
  }

  /**
   * Log a setting to the system_settings log.
   *
   * @param $name
   *   The name of the setting to log.
   * @param $old_value
   *   The old system_setting array.
   * @param $new_value
   *   The new system_setting array.
   * @param $description
   *   A human readible description of the action taken or setting changed.
   */
  public function logSetting($name, $old_value, $new_value, $description) {
    $old_value = serialize($old_value);
    $sql = "INSERT INTO {system_settings_log} 
            (package, name, old_value, new_value, description) 
            VALUES ('%s', '%s', '%s', '%s', '%s')";
    db_query($sql, $package, $name, serialize($old_values['value']), serialize($new_value), $description);
  }
}
