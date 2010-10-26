<?php
// $Id$

/**
 * @file
 *   Provide administrative interfaces for system settings operations.
 */

/**
 * Render the system settings configuration page.
 */
function system_admin_menu_block_page() {
  $output = t('This page has not been completed. !try', array('!try' => l('Try this one!', 'admin/config/system-settings/backends')));
  return $output;
}

/**
 * Provide a form allowing users to initialize a sqlite database for
 * their system_settings and to advice them on adding this to settings.php.
 */
function system_settings_api_initialize_db_form() {
  $form['system_settings_sqlite_db'] = array(
    '#type' => 'textfield',
    '#title' => t('Databse Name'),
    '#default_value' => system_settings_get('system_settings_ui_sqlite_db'),
    '#description' => t('The path at which you would like to initialize a SQLite database separate from the primary Drupal database to store system_settings.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
  );
  // Provide a form for migrating the data to the newly created sqlite database.
  $form['migration'] = array(
    '#type' => 'fieldset',
    '#title' => t('Migration'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['migration']['migrate'] = array(
    '#type' => 'submit',
    '#value' => 'Migrate Content From Variables Table',
    '#submit' => 'system_settings_ui_migrate_variables_table',
  );
  return $form;
}

/**
 *
 */
function system_settings_api_initialize_db_form_validate($form, &$form_state) {
  // TODO
  // Verify that this is a valid path.
  // TODO
  // Verify that we are capable of writing to this path.
}

/**
 * Process the submission form for creating a sqlite database.
 */
function system_settings_api_initialize_db_form_submit($form, &$form_state) {
  //system_settings_set('system_settings_sqlite_db', $form_state['values']['system_settings_sqlite_db'], 'system');
  $file_location = $form_state['values']['system_settings_sqlite_db'];
  // Create the sqlite database and install the schema.
  module_load_include('inc', 'system_settings', 'system_settings.sqlite');
  system_settings_create_sqlite_db($file_location);
  // Redirect to a page that displays the results.
  drupal_set_message(t('Please add the following to your settings.php file: !settings', array('!settings' => $settings_text)));
}
