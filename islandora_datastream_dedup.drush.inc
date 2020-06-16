<?php

function islandora_datastream_dedup_drush_command() {

  $commands['dedup-datastreams'] = array(
    'description' => 'Removes older versions of a datastream, preserving the current version. Additional options to remove all datastreams, preview-only, filter by collection, content model.',
    'aliases' => array('ddds'),
    'options' => array(
      'ds' => array(
        'description' => 'A datastream identifier, e.g. "TN" or "OBJ"',
        'required' => TRUE,
      ),
      'op' => 'Optional: This must be set to "nuke-dups" or "nuke-all" to actually perform deletion of the datastreams. Acceptable values are: "preview-dups" (report on duplicate datastreams), "preview-all" (report on all copies of the datastream), "nuke-dups" (delete duplicate datastreams), and "nuke-all" (delete all copies of the datastream). If not provided, defaults to "preview-dups". ',
      'mimetype' => array(
        'description' => 'Optional: The mimetype that the datastream uses. E.g. "image/jpeg", or "image/tiff"',
      ),
      'cm' => 'Optional: Comma-separated list of content model pids. Restricts datastream deduping to objects that match these content-models. E.g. `--cm=islandora:newspaperPageCModel,islandora:pageCModel',
      'collection-pids-file' => 'Optional: Provide path to a file with collection pids to exclude children of. One pid per line.',
      'collection-pids' => 'Optional: Provide one or more collection pids to include children of. Separate multiple pids with commas',
      'exclude-pids' => 'Optional: Negate the collection pids, i.e. exclude children of the pid/pids provided by collection-pids-file and collection-pid.',
      'timer' => 'Optional: Output time stats.',
    ),
    'examples' => array(
      'drush ddds -u 1 --ds=OBJ --mimetype="image/jpeg" --op=nuke-dups --timer' => 'Delete duplicate OBJ datastreams whose mimetype is "image/jpeg", and show timing statistics.',
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
  $op = drush_get_option('op');
  $op = $op ? $op: 'preview-dups';
  $op = explode('-', $op);
  $op = array('mode' => $op[0], 'scope' => $op[1]);
  $options = array(
    'collection-pids-file' => drush_get_option('collection-pids-file'),
    'exclude-pids' => drush_get_option('exclude-pids'),
    'collection_pids' => drush_get_option('collection-pids'),
    'timer' => drush_get_option('timer'),
    'mimetype' => $mimetype = drush_get_option('mimetype'),
    'cm' => drush_get_option('cm'),
  );
  $options = array_filter($options);
  module_load_include('inc', 'islandora_datastream_dedup', 'includes/utilities');
  _datastream_dedup($op, $ds, $options);
  if(!empty($options['timer'])) {
    drush_print_timers();
  }
}
