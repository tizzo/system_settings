<?php
// $Id$

/**
 * @file
 *   Defines the default handler system_settings storage backends.
 */

/**
 * Provides the default handler for storing system_settings in a 
 * relational database.
 */
 // TODO: Implement the settings inteface once the API is almost finished.
class SystemSettingsDefaultHandler /*implements SystemSettingsInterface*/ {

  // Internal copy of the global $system_settings for testing purposes.
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
    $results = dbtng_query("SELECT name, value FROM {system_settings} WHERE always_load=:always_load", array(':always_load' => $always_load))->fetchAll();
    foreach ($results as $result) {
      $setting_array['name'] = $result->name;
      $setting_array['value'] = unserialize($result->value);
      foreach (system_settings_get_relation_types('names') as $relation_type_name) {
        if ($relation_data = $this->getRelationsForSettingByRelationType($relation_type_name, $result->name)) {
          $setting_array = system_settings_array_merge_recursive_distinct($setting_array, $relation_data);
        }
      }
      _system_settings_process_setting_array($setting_array['name'], $setting_array, $system_settings) ;
    }
  }

  /**
   * Get a list of all relations in the system setting API.
   */
  public function getAllRelations() {
    $relations = array();
    // TODO: Write this function.
    return $relations;
  }

  /**
   * Get the settings for a given relation.
   *
   * @param $relation_type_name
   *   The type of the relation for which to get relations.
   * @param $name (optional)
   *   The name of the setting for which to retrieve relations.
   */
  public function getRelationsForSettingByRelationType($relation_type_name, $name = FALSE) {
    $relations = array();
    $relation_types = system_settings_get_relation_types();
    $fields = $this->getFieldsForRelationType($relation_type_name);
    // Unset the name field, loading it
    // in the relation array would be redundant.
    unset($fields['name']);
    $query = dbtng_select('system_settings_' . $relation_type_name, 'ssrtn')
      ->fields('ssrtn', $fields);
    if ($name) { 
      $query->condition('name', $name);
    }
    $results = $query->execute()
      ->fetchAll();
    $relation_types = system_settings_get_relation_types();
    $identifier_names = $relation_types[$relation_type_name]['identifiers'];
    foreach ($results as $result) {
      $identifiers[] = array();
      foreach ($identifier_names as $identifier) {
        $identifiers[] = $result->$identifier;
      }
      $relations[$relation_type_name][implode('][', $identifiers)] = (array) $result;
    }
    return $relations;
  }

  /**
   * Retrieve an array of all entries for a type or relation.
   */
  public function getRelationEntriesByRelationType($relation_type_name) {
    $relation_types = system_settings_get_relation_types();
    $identifiers = $relation_types[$relation_type_name]['identifiers'];
    static $cached_relations = array();
    if (!isset($cached_relations[$relation_type_name])) {
      $cached_relations[$relation_type_name] = array();
      $fields = $this->getFieldsForRelationType($relation_type_name);
      $query = dbtng_select('system_settings_' . $relation_type_name, 'ssrtn')
        ->fields('ssrtn', $fields);
      foreach ($fields as $field) {
        $query->groupBy('ssrtn.' . $field);
      }
      $results = $query->execute()
        ->fetchAll();
      foreach ($results as $result) {
        $identifier_values = array_intersect($identifiers, array_keys((array) $result));
        $identifiers = array_combine($identifiers, $identifier_values);
        $cached_relations[$relation_type_name][implode('][', $identifiers)] = (array) $result;
      }
    }
    return $cached_relations[$relation_type_name];
  }

  /**
   * TODO: Document this more.
   * @param (mixed) $relation
   *   The name of the group to load settings for.
   * @return (array)
   *   An array of the settings relevant to this group.
   */
  public function getSettingsByRelation($relation_type_name, $relation, $always_load = TRUE) {
    // TODO: This is completely untested.
    static $cached_settings = array();
    $relation_types = system_settings_get_relation_types();
    $identifiers = $relation_types[$relation_type_name]['identifiers'];
    if (is_string($relation)) {
      $relation_identifier = $relation;
      $relation = array_combine($identifiers, explode('][', $identifiers));
    }
    else {
      $relation_identifier = implode('][', $identifiers);
    }
    if (!$always_load && isset($cached_settings[$relation_type_name][$relation_identifier])) {
      return $cached_settings[$relation_type_name][$relation_identifier];
    }
    else {
      $query = dbtng_select('system_settings', 'ss')
        ->fields('ss', array('name', 'value'))
        ->join('system_settings_' . $relation_type_name, 'ssrtn', 'ss.name = ssrtn.name');
      foreach ($relation as $relation_identifier_name => $relation_identifier_value) {
        if (in_array($relation_identifier_name, $identifiers)) {
          $query->condition($relation_identifier_name, $relation_identifier_value);
        }
      }
      $results = $query->condition('ss.always_load', $always_load)
        ->execute()
        ->fetchAll();
      foreach ($results as $result) {
        $cached_settings[$relation_type_name][$relation_identifier] = (array) $result;
      }
      return $cached_settings[$relation_type_name][$relation_identifier];
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
   *
   */
  public function removeSettingsByRelation($relation_type, $relation_identifiers = array()) {
  }

  /**
   * Load an individual system_setting array by name.
   */
  public function getSetting($name) {
    $setting = dbtng_query("SELECT name, value, always_load FROM {system_settings} WHERE name = :name", array(':name' => $name))->fetchCol;
    $setting['value'] = unserialize($setting->value);
    $relations = $this->getSettingRelations($name);
    $setting = array_merge($setting, $relations);
    return $setting;
  }

  /**
   * Load all of the Relations related to a given setting.
   */
  public function getSettingRelations($name) {
    $relations = array();
    $relation_types = system_settings_get_relation_types();
    foreach ($relation_types as $relation_type_name => $identifiers) {
      $identifiers = $identifiers['identifiers'];
      $results = dbtng_select('system_settings_' . $relation_type_name, 'rtn')
        ->fields('rtn', $identifiers)
        ->execute();
      while ($result = $results->fetchcol()) {
        $identifier = implode('][', $identifiers);
        $relations[$relation_type_name][$identifier] = $result;
      }
    }
    return $relations;
  }

  /**
   * Store a system setting.
   *  //TODO, Rework this!
   */
  public function storeSetting($setting_array) {
    $schema = drupal_get_schema('system_settings');
    $fields = $schema['fields'];
    $record = new stdClass;
    foreach ($setting_array as $name => $value) {
      if (array_key_exists($name, $fields)) {
        $record->$name = $value;
      }
    }
    $record->value = serialize($record->value);
    $sql = "SELECT name FROM {system_settings} WHERE name=:name";
    if (dbtng_query($sql, array(':name' => $setting_array['name']))->fetchCol()) {
      $update = 'name';
    }
    drupal_write_record('system_settings', $record, $update);
    $this->storeRelations($setting_array);
  }

  /**
   * Store all relations related to a given system_setting.
   *
   * @param $system_setting
   *   A single system_setting array.
   */
  private function storeRelations($setting_array) {
    $relation_types = system_settings_get_relation_types();
    foreach ($setting_array as $relation_type_name => $relations) {
      if (array_key_exists($relation_type_name, $relation_types)) {
        $table = "system_settings_$relation_type_name";
        $schema = drupal_get_schema($table);
        $fields = array_keys($schema['fields']);
        foreach ($relations as $relation) {
          $query = dbtng_select($table, 'rtn')
            ->fields('rtn', $fields);
          $identifiers = array();
          foreach ($relation as $identifier_name => $identifier) {
            if (in_array($identifier_name, $relation_types[$relation_type_name]['identifiers'])) {
              $query->condition($identifier_name, $identifier);
              $identifiers[] = $identifier_name;
            }
          }
          if ($query->execute()->fetchcol()) {
            $primary_keys = $identifiers;
          }
          $relation['name'] = $setting_array['name'];
          drupal_write_record($table, $relation, $primary_keys);
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
    $fields = array_keys($schema['fields']);
    return $fields;
  }

}
