<?php
// $Id$

/**
 * @file
 * Provide API functions for working with sqlite databases.
 */

/**
 * Create a sqlite database file on the local disk and install the schema.
 *
 * @param
 */
function system_settings_create_sqlite_db($file_location) {
  // Create database information.
  $key = 'system_settings';
  $target = 'default';
  $db_info = array(
    'driver' => 'sqlite',
    'file' => $file_location,
  );
  // Create the database
  $database = new SQLiteDatabase($file_location, 0666, $error);
  // Modify the database info so that the db layer knows 
  // what to do with our new database.
  Database::addConnectionInfo($key, $target, $db_info);
  // Load our new database information.
  Database::parseConnectionInfo();
  // Connect to the database.
  $db = Database::getConnection($target, $key);
  // Install the schema.
  // Get the system_settings schema.
  $schema = drupal_get_schema_unprocessed('system_settings');
  _drupal_schema_initialize('system_settings', $schema);
  // TODO: load the schema from relations defined by other modules
  foreach ($schema as $name => $table) {
    $db->schema()->createTable($name, $table);
  }
  // TODO: Make this not lie (verify it worked)
  return TRUE;
}

