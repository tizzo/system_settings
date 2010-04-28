<?php
// $Id$

/**
 * @file
 * Administrative page to provide a recovery console to hanlde 
 * handle rolling back from one configuration of Drupal to another.
 *
 * Point your browser to "http://www.example.com/[path-to-module]/recovery.php" and follow the
 * instructions.
 *
 * If you are not logged in as administrator, you will need to modify the access
 * check statement inside your settings.php file. After finishing the upgrade,
 * be sure to open settings.php again, and change it back to its original state!
 */

/**
 * TODO: This was lifted from update.php
 * Som of the extraneous code has been removed.
 * Remove more, actually make it work?
 */

/**
 * Global flag to identify recovery.php run, and so avoid various unwanted
 * operations, such as hook_init() and hook_exit() invokes, css/js preprocessing
 * and translation, and solve some theming issues. This flag is checked on several
 * places in Drupal code (not just update.php).
 */
define('MAINTENANCE_MODE', 'update');

function update_selection_page() {
  $output = '<p>The version of Drupal you are updating from has been automatically detected. You can select a different version, but you should not need to.</p>';
  $output .= '<p>Click Update to start the update process.</p>';

  drupal_set_title('Drupal database update');
  $output .= drupal_get_form('update_script_selection_form');

  update_task_list('select');

  return $output;
}

function update_script_selection_form() {
  $form = array();
  $form['start'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => 'Select versions',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // Ensure system.module's updates appear first
  $form['start']['system'] = array();

  $modules = drupal_get_installed_schema_version(NULL, FALSE, TRUE);
  foreach ($modules as $module => $schema_version) {
    $updates = drupal_get_schema_versions($module);
    // Skip incompatible module updates completely, otherwise test schema versions.
    if (!update_check_incompatibility($module) && $updates !== FALSE && $schema_version >= 0) {
      // module_invoke returns NULL for nonexisting hooks, so if no updates
      // are removed, it will == 0.
      $last_removed = module_invoke($module, 'update_last_removed');
      if ($schema_version < $last_removed) {
        $form['start'][$module] = array(
          '#value'  => '<em>'. $module .'</em> module can not be updated. Its schema version is '. $schema_version .'. Updates up to and including '. $last_removed .' have been removed in this release. In order to update <em>'. $module .'</em> module, you will first <a href="http://drupal.org/upgrade">need to upgrade</a> to the last version in which these updates were available.',
          '#prefix' => '<div class="warning">',
          '#suffix' => '</div>',
        );
        $form['start']['#collapsed'] = FALSE;
        continue;
      }
      $updates = drupal_map_assoc($updates);
      $updates[] = 'No updates available';
      $default = $schema_version;
      foreach (array_keys($updates) as $update) {
        if ($update > $schema_version) {
          $default = $update;
          break;
        }
      }
      $form['start'][$module] = array(
        '#type' => 'select',
        '#title' => $module .' module',
        '#default_value' => $default,
        '#options' => $updates,
      );
    }
  }

  $form['has_js'] = array(
    '#type' => 'hidden',
    '#default_value' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Update',
  );
  return $form;
}

function update_batch() {
  global $base_url;

  $operations = array();
  // Set the installed version so updates start at the correct place.
  foreach ($_POST['start'] as $module => $version) {
    drupal_set_installed_schema_version($module, $version - 1);
    $updates = drupal_get_schema_versions($module);
    $max_version = max($updates);
    if ($version <= $max_version) {
      foreach ($updates as $update) {
        if ($update >= $version) {
          $operations[] = array('update_do_one', array($module, $update));
        }
      }
    }
  }
  $batch = array(
    'operations' => $operations,
    'title' => 'Updating',
    'init_message' => 'Starting updates',
    'error_message' => 'An unrecoverable error has occurred. You can find the error message below. It is advised to copy it to the clipboard for reference.',
    'finished' => 'update_finished',
  );
  batch_set($batch);
  batch_process($base_url .'/update.php?op=results', $base_url .'/update.php');
}

function update_finished($success, $results, $operations) {
  // clear the caches in case the data has been updated.
  drupal_flush_all_caches();

  $_SESSION['update_results'] = $results;
  $_SESSION['update_success'] = $success;
  $_SESSION['updates_remaining'] = $operations;
}

function update_results_page() {
  drupal_set_title('Drupal database update');
  // NOTE: we can't use l() here because the URL would point to 'update.php?q=admin'.
  $links[] = '<a href="'. base_path() .'">Main page</a>';
  $links[] = '<a href="'. base_path() .'?q=admin">Administration pages</a>';

  update_task_list();
  // Report end result
  if (module_exists('dblog')) {
    $log_message = ' All errors have been <a href="'. base_path() .'?q=admin/reports/dblog">logged</a>.';
  }
  else {
    $log_message = ' All errors have been logged.';
  }

  if ($_SESSION['update_success']) {
    $output = '<p>Updates were attempted. If you see no failures below, you may proceed happily to the <a href="'. base_path() .'?q=admin">administration pages</a>. Otherwise, you may need to update your database manually.'. $log_message .'</p>';
  }
  else {
    list($module, $version) = array_pop(reset($_SESSION['updates_remaining']));
    $output = '<p class="error">The update process was aborted prematurely while running <strong>update #'. $version .' in '. $module .'.module</strong>.'. $log_message;
    if (module_exists('dblog')) {
      $output .= ' You may need to check the <code>watchdog</code> database table manually.';
    }
    $output .= '</p>';
  }

  if (!empty($GLOBALS['update_free_access'])) {
    $output .= "<p><strong>Reminder: don't forget to set the <code>\$update_free_access</code> value in your <code>settings.php</code> file back to <code>FALSE</code>.</strong></p>";
  }

  $output .= theme('item_list', $links);

  // Output a list of queries executed
  if (!empty($_SESSION['update_results'])) {
    $output .= '<div id="update-results">';
    $output .= '<h2>The following queries were executed</h2>';
    foreach ($_SESSION['update_results'] as $module => $updates) {
      $output .= '<h3>'. $module .' module</h3>';
      foreach ($updates as $number => $queries) {
        if ($number != '#abort') {
          $output .= '<h4>Update #'. $number .'</h4>';
          $output .= '<ul>';
          foreach ($queries as $query) {
            if ($query['success']) {
              $output .= '<li class="success">'. $query['query'] .'</li>';
            }
            else {
              $output .= '<li class="failure"><strong>Failed:</strong> '. $query['query'] .'</li>';
            }
          }
          if (!count($queries)) {
            $output .= '<li class="none">No queries</li>';
          }
        }
        $output .= '</ul>';
      }
    }
    $output .= '</div>';
  }
  unset($_SESSION['update_results']);
  unset($_SESSION['update_success']);

  return $output;
}

function update_info_page() {
  // Change query-strings on css/js files to enforce reload for all users.
  _drupal_flush_css_js();
  // Flush the cache of all data for the update status module.
  if (db_table_exists('cache_update')) {
    cache_clear_all('*', 'cache_update', TRUE);
  }

  update_task_list('info');
  drupal_set_title('Drupal database update');
  $token = drupal_get_token('update');
  $output = '<p>Use this utility to update your database whenever a new release of Drupal or a module is installed.</p><p>For more detailed information, see the <a href="http://drupal.org/node/258">Installation and upgrading handbook</a>. If you are unsure what these terms mean you should probably contact your hosting provider.</p>';
  $output .= "<ol>\n";
  $output .= "<li><strong>Back up your database</strong>. This process will change your database values and in case of emergency you may need to revert to a backup.</li>\n";
  $output .= "<li><strong>Back up your code</strong>. Hint: when backing up module code, do not leave that backup in the 'modules' or 'sites/*/modules' directories as this may confuse Drupal's auto-discovery mechanism.</li>\n";
  $output .= '<li>Put your site into <a href="'. base_path() .'?q=admin/settings/site-maintenance">maintenance mode</a>.</li>'."\n";
  $output .= "<li>Install your new files in the appropriate location, as described in the handbook.</li>\n";
  $output .= "</ol>\n";
  $output .= "<p>When you have performed the steps above, you may proceed.</p>\n";
  $output .= '<form method="post" action="update.php?op=selection&amp;token='. $token .'"><p><input type="submit" value="Continue" /></p></form>';
  $output .= "\n";
  return $output;
}

function update_access_denied_page() {
  drupal_set_title('Access denied');
  return '<p>Access denied. You are not authorized to access this page. Please log in as the admin user (the first user you created). If you cannot log in, you will have to edit <code>settings.php</code> to bypass this access check. To do this:</p>
<ol>
 <li>With a text editor find the settings.php file on your system. From the main Drupal directory that you installed all the files into, go to <code>sites/your_site_name</code> if such directory exists, or else to <code>sites/default</code> which applies otherwise.</li>
 <li>There is a line inside your settings.php file that says <code>$update_free_access = FALSE;</code>. Change it to <code>$update_free_access = TRUE;</code>.</li>
 <li>As soon as the update.php script is done, you must change the settings.php file back to its original form with <code>$update_free_access = FALSE;</code>.</li>
 <li>To avoid having this problem in future, remember to log in to your website as the admin user (the user you first created) before you backup your database at the beginning of the update process.</li>
</ol>';
}

/**
 * Create the batch table.
 *
 * This is part of the Drupal 5.x to 6.x migration.
 */
function update_create_batch_table() {

  // If batch table exists, update is not necessary
  if (db_table_exists('batch')) {
    return;
  }

  $schema['batch'] = array(
    'fields' => array(
      'bid'       => array('type' => 'serial', 'unsigned' => TRUE, 'not null' => TRUE),
      'token'     => array('type' => 'varchar', 'length' => 64, 'not null' => TRUE),
      'timestamp' => array('type' => 'int', 'not null' => TRUE),
      'batch'     => array('type' => 'text', 'not null' => FALSE, 'size' => 'big')
    ),
    'primary key' => array('bid'),
    'indexes' => array('token' => array('token')),
  );

  $ret = array();
  db_create_table($ret, 'batch', $schema['batch']);
  return $ret;
}

/**
 * Disable anything in the {system} table that is not compatible with the
 * current version of Drupal core.
 */
function update_fix_compatibility() {
  $ret = array();
  $incompatible = array();
  $query = db_query("SELECT name, type, status FROM {system} WHERE status = 1 AND type IN ('module','theme')");
  while ($result = db_fetch_object($query)) {
    if (update_check_incompatibility($result->name, $result->type)) {
      $incompatible[] = $result->name;
    }
  }
  if (!empty($incompatible)) {
    $ret[] = update_sql("UPDATE {system} SET status = 0 WHERE name IN ('". implode("','", $incompatible) ."')");
  }
  return $ret;
}

/**
 * Helper function to test compatibility of a module or theme.
 */
function update_check_incompatibility($name, $type = 'module') {
  static $themes, $modules;

  // Store values of expensive functions for future use.
  if (empty($themes) || empty($modules)) {
    $themes = _system_theme_data();
    $modules = module_rebuild_cache();
  }

  if ($type == 'module' && isset($modules[$name])) {
    $file = $modules[$name];
  }
  else if ($type == 'theme' && isset($themes[$name])) {
    $file = $themes[$name];
  }
  if (!isset($file)
      || !isset($file->info['core'])
      || $file->info['core'] != DRUPAL_CORE_COMPATIBILITY
      || version_compare(phpversion(), $file->info['php']) < 0) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Add the update task list to the current page.
 */
function update_task_list($active = NULL) {
  // Default list of tasks.
  $tasks = array(
    'info' => 'Overview',
    'select' => 'Select updates',
    'run' => 'Run updates',
    'finished' => 'Review log',
  );

  drupal_set_content('left', theme('task_list', $tasks, $active));
}

require_once './includes/bootstrap.inc';

// We only load DRUPAL_BOOTSTRAP_CONFIGURATION for the update requirements
// check to avoid reaching the PHP memory limit.
$op = isset($_REQUEST['op']) ? $_REQUEST['op'] : '';
if (empty($op)) {
  // Minimum load of components.
  drupal_bootstrap(DRUPAL_BOOTSTRAP_CONFIGURATION);

  require_once './includes/install.inc';
  require_once './includes/file.inc';
  require_once './modules/system/system.install';

  // Load module basics.
  include_once './includes/module.inc';
  $module_list['system']['filename'] = 'modules/system/system.module';
  $module_list['filter']['filename'] = 'modules/filter/filter.module';
  module_list(TRUE, FALSE, FALSE, $module_list);
  drupal_load('module', 'system');
  drupal_load('module', 'filter');

  // Set up $language, since the installer components require it.
  drupal_init_language();

  // Set up theme system for the maintenance page.
  drupal_maintenance_theme();

  // Check the update requirements for Drupal.
  update_check_requirements();

  // Display the warning messages (if any) in a dedicated maintenance page,
  // or redirect to the update information page if no message.
  $messages = drupal_set_message();
  if (!empty($messages['warning'])) {
    drupal_maintenance_theme();
    print theme('update_page', '<form method="post" action="update.php?op=info"><input type="submit" value="Continue" /></form>', FALSE);
    exit;
  }
  install_goto('update.php?op=info');
}

drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
drupal_maintenance_theme();

// This must happen *after* drupal_bootstrap(), since it calls
// variable_(get|set), which only works after a full bootstrap.
update_create_batch_table();

// Turn error reporting back on. From now on, only fatal errors (which are
// not passed through the error handler) will cause a message to be printed.
ini_set('display_errors', TRUE);

// Access check:
if (!empty($update_free_access) || $user->uid == 1) {

  include_once './includes/install.inc';
  include_once './includes/batch.inc';
  drupal_load_updates();

  $op = isset($_REQUEST['op']) ? $_REQUEST['op'] : '';
  switch ($op) {
    case 'selection':
      if (isset($_GET['token']) && $_GET['token'] == drupal_get_token('update')) {
        $output = update_selection_page();
        break;
      }

    case 'Update':
      if (isset($_GET['token']) && $_GET['token'] == drupal_get_token('update')) {
        update_batch();
        break;
      }

    // update.php ops
    case 'info':
      $output = update_info_page();
      break;

    case 'results':
      $output = update_results_page();
      break;

    // Regular batch ops : defer to batch processing API
    default:
      update_task_list('run');
      $output = _batch_page();
      break;
  }
}
else {
  $output = update_access_denied_page();
}
if (isset($output) && $output) {
  // We defer the display of messages until all updates are done.
  $progress_page = ($batch = batch_get()) && isset($batch['running']);
  print theme('update_page', $output, !$progress_page);
}
