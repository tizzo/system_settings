<?php
/**
 * @file
 *   
 * @param $key
 *
 * @param $target
 *
 * @param $path
 * 
 */
?>
$databases['<?php print $key; ?>'] = array(
  'default' => array (
    'driver' => 'sqlite',
    'file' => '<?php print $path; ?>',
  ),    
);      
$conf['system_settings_conf'] = array(
  'system_settings_handlers' =>array(
    'modules' => array('system_settings'),
    'value' => array(
      'system_settings' => array(
        'handler' => 'default',
        'conf' => array (
          'db_key' => '<?php print $key; ?>',
          'db_target' => '<?php print $target ?>',
         ),
      ),
    ),
  ),
);
