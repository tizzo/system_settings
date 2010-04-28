<?php
// $Id$

/**
 * @file
 *   Defines the default handler system_settings storage backends.
 */

/**
 * Provides the default handler for storing system_settings.
 * TODO: Implement the settings inteface once the API is almost finished.
 */
class SystemSettingsDefaultHandler /*implements SystemSettingsInterface*/ {

  // Internal copy of the global $system_settings
  protected  $system_settings;

  /**
   * Configure this object using a config object.
   *
   * @param $config (mixed)
   *   An object or array with keys for the object.
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
  public function getAllSettings(&$system_settings, $always_load = TRUE) {
    $result = db_query("SELECT name, value FROM {system_settings} WHERE always_load=%d", $always_load);
    while ($setting_array = db_fetch_array($result)) {
      foreach (system_settings_get_relation_types('names') as $relation_type) {
        $relation_data = $this->getRelationsForSettingByRelationType($relation_type, $result->name);
        $setting_array[$relation_type] = $relation_data;
      }
      _system_settings_process_setting_array($setting_array['name'], $setting_array, $system_settings) ;
    }
  }

  /**
   * Get a list of all relations in the system setting API.
   */
  public function getAllRelations() {
    return $relations;
  }

  /**
   * Get the settings for a given relation.
   *
   * @param $relation
   *   The type of the relation for which to get relations.
   * @param $name (optional)
   *   The name of the setting for which to retrieve relations.
   */
  public function getRelationsForSettingByRelationType($relation_type, $name = FALSE) {
    $relation_types = system_settings_get_relation_types();
    $fields = $this->getFieldsForRelationType($relation_types);
    $fields = implode(', ', array_keys($fields));
    $sql .= "SELECT $fields FROM {system_settings_$relation_type}";
    if ($name) { 
      " WHERE name = '%s'";
    }
    $result = db_query($sql, $name);
    $relation_types = system_settings_get_relation_types();
    $identifier_fields = $relation_types[$relation_type]['identifiers'];
    while ($item = db_fetch_array($result)) {
      foreach ($identifier_fields as $identifier_field) {
        $relations[$relation_type][$item[$identifier_field]] = $item;
      }
    }
    return $relations;
  }

  /**
   * Retrieve a linear array of all entries for a type or relation.
   */
  public function getRelationEntriesByRelationType($relation_type) {
    $relation_types = system_settings_get_relation_types();
    static $cached_relations = array();
    if (!$cached_relations[$relation_type]) {
      $relations[$relation] = array();
      $fields = $this->getFieldsForRelationType($relation_type);
      $fields = implode(', ', array_keys($fields));
      $result = db_query("SELECT $fields FROM {system_settings_$relation_type} GROUP BY $fields");
      while ($item = db_result($result)) {
        $relations[$relation] = $item;
      }
    }
    return $relations[$relation];
  }

  /**
   * @param (string) $group
   *   The name of the group to load settings for.
   * @return (array)
   *   An array of the settings relevant to this group.
   */
  // TODO: this is porbably broken... work on this...
  public function getSettingsByRelation($relation, $alway_load = TRUE) {
    if (!$active && isset($system_settings[$relation])) {
      return $system_settings[$relation];
    }
    else {
      $sql = "SELECT ss.name, ss.value
              FROM {system_settings} as ss
              JOIN {system_settings_$relation} AS ssr ON ss.name = ssr.name
              WHERE ssr.group = %s AND 'always_load' = %d";
      $result = db_query($sql, $relation, $always_load);
      $settings = array();
      while ($setting = db_fetch_object($result)) {
        $settings[$setting->name] = $setting->value;
      }
      return $settings;
    }
  }

  /**
   * Cleanup orphaned settings that are no longer 
   * associated with any module or theme.
   */
  public function removeOrphanSettings() {
    // TODO: Write this function.
    return FALSE;
  }

  /**
   * Load an individual system_setting array by name.
   */
  public function getSetting($name) {
    $setting = db_fetch_result(db_query("SELECT name, value, always_load FROM {system_settings} WHERE name = '%s'", $name));
    $setting['value'] = unserialize($setting['value']);
    $entities = $this->getSettingEntities($name);
    $setting = array_merge($setting, $entities);
    return $setting;
  }

  /**
   * Load all of the Relations related to a given setting.
   */
  public function getSettingRelations($name) {
    $entities = array();
    $relations = system_settings_get_relation_types();
    foreach ($relations as $relation) {
      $entities[$relation] = $this->getSettingGroups($name);
    }
    return $entities;
  }

  /**
   * Store a system setting.
   *  //TODO, Rework this!
   */
  public function storeSetting($setting) {
    // Store or update the setting and value
    if (!db_fetch_result(db_query("SELECT name FROM {system_settings} WHERE name='%s'", $setting['name']))) {
      $update = '';
      $sql = "INSERT INTO {system_settings} 
              (name, value, load_frequency) 
              VALUES ('%s', '%s', %d)";
      db_query($sql, $setting['name'], serialize($value), $setting['load']);
    }
    else {
      $sql = "UPDATE {system_settings} 
              SET value = %s, load_frequency = %s 
              WHERE name = '%s'";
      db_query($sql, serialize($setting['value']), $load_frequency, $name);
    }
  }

  /**
   * Store all relations related to a given system_setting.
   *
   * @param $system_setting
   *   A single system_setting array.
   */
   // TODO: This is waaaaaaay broken!
  private function storeRelations($system_setting) {
    $relation_types = system_settings_get_relation_types();
    foreach($relation_types as $relation_type) {
      foreach ($system_setting as $relation_name => $relation_data) {
        if (in_array($relation_name, array_keys($relation_types))) {
          $sql = "SELECT name FROM {system_settings_$table}
                  WHERE name = '%s'
                  AND $relation = '%s'";
          if(db_result(db_query($sql, $system_setting['name'], $system_setting[$relation]))) {
            $update = $identifiers;
          }
          drupal_write_record($table, $relation_data, $update);
        }
      }
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
    $data = new stdClass;
    $data->old_value = serialize($old_value);
    $data->new_value = serialize($new_value);
    $data->description = $description;
    drupal_write_record('system_settings_log', $data);
  }

  /**
   * Internal function specific to the default relational db handler.
   */
  private function getFieldsForRelationType($relation_type) {
    $relation_types = system_settings_get_relation_types();
    $schema = drupal_get_schema("system_settings_$relation_type");
    $fields = $schema['fields'];
    return $fields;
  }

}
