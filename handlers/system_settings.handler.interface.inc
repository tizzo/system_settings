<?php
/**
 * @file
 *   Defines the interface for system settings backends
 */
interface SystemSettingsInterface {
  public function get($cid);
  public function set($cid, $data, $expire = CACHE_PERMANENT, $headers = NULL);
  public function clear($cid = NULL, $table = NULL, $wildcard = FALSE);
}
