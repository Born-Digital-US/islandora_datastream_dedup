<?php

function islandora_datastream_dedup_drush_command() {

  $commands['dedup-datastreams'] = array(
    'description' => 'Removes older versions of a datastream, preserving the current version',
    'aliases' => array('ddds'),
    'options' => array(
      'ds' => array(
        'description' => 'A datastream identifier, e.g. "TN" or "OBJ"',
        'required' => TRUE,
      ),
      'mimetype' => array(
        'description' => 'The mimetype that the datastream uses. E.g. "image/jpeg", or "image/tiff"',
        'required' => TRUE,
      ),
      'nuke' => 'Optional: This flag must be set to actually perform deletion of the datastreams. Otherwise you will only see a report that previews what would have been deleted.',
      'timer' => 'Optional: Output time stats.',
    ),
    'examples' => array(
      'drush ddds -u 1 --ds=OBJ --mimetype="image/jpeg" --nuke --timer' => 'Delete duplicate OBJ datastreams whose mimetype is "image/jpeg", and show timing statistics.',
      'drush ddds  -u 1 --ds=OBJ --mimetype="image/tiff"' => 'Display stats about duplicate OBJ datastreams where the mimetype is "image/tiff". No datastreams will be deleted',
    ),
  );

  return $commands;
}

/**
 * Drush command logic.
 * drush_[COMMAND_NAME]().
 */
function drush_islandora_datastream_dedup_dedup_datastreams() {
  $ds = drush_get_option('ds', 'OBJ');
  $mimetype = drush_get_option('mimetype', 'image/tiff');
  $op = (drush_get_option('nuke') ? 'nuke': 'preview');
  $timer = drush_get_option('timer');
  module_load_include('inc', 'islandora_datastream_dedup', 'includes/utilities');
  _datastream_dedup($mimetype, $op, $ds, $timer);
  if($timer) {
    drush_print_timers();
  }
}
