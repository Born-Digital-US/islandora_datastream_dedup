<?php

function _datastream_dedup($op = 'preview', $dsid = 'OBJ', $options = array()) {

  $query = array();
  $timer = FALSE;

  $data = str_getcsv($dsid);
  $data = array_map('trim',$data);
  $dsids = array_filter($data);


  $collection_stats_record_template = [
    'object count' => 0,
    'datastream count' => 0,
    'datastream total size' => 0,
    'first datastream total size' => 0,
    'datastreams deleted count' => 0,
    'datastreams deleted total size' => 0,
  ];
  $collection_stats = ['All Objects' => $collection_stats_record_template];
  foreach($dsids as $dsid) {
    $collection_stats['All Objects'][$dsid] = $collection_stats_record_template;
  }

  $pids = array();
  $exclude = '';
  foreach($options as $key => $value) {
    switch($key) {
      case 'mimetype':
        $mimetype = $value;
        $query[] = 'fedora_datastream_latest_' . $dsid . '_MIMETYPE_mt:"' . $mimetype . '"';
      case 'timer':
        $timer = TRUE;
        break;
      case 'collection-pids-file':
        $fp = fopen($value, 'r');
        if($fp) {
          while (($data = fgetcsv($fp)) !== FALSE) {
            $data = array_map('trim',$data);
            $data = array_filter($data);
            if(!empty($data)) {
              $pids = array_merge($pids, $data);
            }
          }
          fclose($fp);
        }
        break;
      case 'exclude-pids':
        $exclude = '-';
        break;
      case 'collection_pids':
        $data = str_getcsv($value);
        $data = array_map('trim',$data);
        $data = array_filter($data);
        $pids = array_merge($pids, $data);
        break;
    }
  }

  $pids = array_unique($pids);



  if(!empty($pids)) {
    $query_pids = array();
    foreach($pids as $i => $pid) {
      $query_pids[] = 'RELS_EXT_isMemberOfCollection_uri_s:"info:fedora/' . $pid . '"';

      foreach($dsids as $dsid) {
        $collection_stats[$pid][$dsid] = $collection_stats_record_template;
      }
    }
    $query[] = $exclude . '(' . implode(' OR ',$query_pids) . ')';
  }

  $query = implode(' AND ', $query);

  $qp = new IslandoraSolrQueryProcessor();

  $fl = array(
    'PID',
    'fgs_label_s',
  );
  foreach($dsids as $dsid_value) {
    $fl[] = 'RELS_EXT_isMemberOfCollection_uri_s, fedora_datastream_version_' . $dsid_value . '_SIZE_ms';
  }

  $qp->buildQuery($query);
  $qp->solrParams['fl'] = implode(', ', $fl);
  $qp->solrStart = 0;
  $qp->solrLimit = 1;
  if($timer) timer_start('ddds_execute_solr_query');
  $qp->executeQuery(FALSE);
  if($timer) timer_stop('ddds_execute_solr_query');

  $range = 500;
  $t = drupal_is_cli() ? 'dt' : 't';

  if($op == 'nuke' && !user_access('delete fedora objects and datastreams')) {
    $msg = $t("The current user does not have permission to delete datastreams.\nIf running under drush, consider adding '-u 1' to the command.\nProceeding to run in preview mode.\n");
    if(drupal_is_cli()) {
      drush_log("\n" . $msg, 'warning');
    }
    else {
      drupal_set_message($msg,'warning');
    }

  }
  if (!empty($qp->islandoraSolrResult['response']['numFound'])) {
    if(drupal_is_cli()) {
      $msg = $t("Preparing to inspect @num objects for duplicate @dsid datastreams@type.", array(
        '@num' => $qp->islandoraSolrResult['response']['numFound'],
        '@dsid' => $dsid,
        '@type' => !empty($mimetype) ? t(' matching the "@mimetype" mimetype', array('@mimetype' => $mimetype)) : '',
      ));
      drush_log($msg, 'ok');
    }
    do {
      $qp->solrLimit = $range;
      if($timer) timer_start('ddds_execute_solr_query');
      $qp->executeQuery(FALSE);
      if($timer) timer_stop('ddds_execute_solr_query');
      foreach ($qp->islandoraSolrResult['response']['objects'] as $object_result) {
        $collection_stats['All Objects']['object count']++;
        $collection_stats[$pid]['object count']++;
        $pid = str_replace('info:fedora/', '', $object_result['solr_doc']['RELS_EXT_isMemberOfCollection_uri_s']);
        foreach($dsids as $dsid) {
          if (!empty($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'])) {

            $first_datastream_size = reset($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']);
            $collection_stats['All Objects'][$dsid]['first datastream total size'] += $first_datastream_size;
            $collection_stats[$pid][$dsid]['first datastream total size'] += $first_datastream_size;
            foreach ($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'] as $ds_size) {
              $collection_stats['All Objects'][$dsid]['datastream count']++;
              $collection_stats[$pid][$dsid]['datastream count']++;
              $collection_stats['All Objects'][$dsid]['datastream total size'] += $ds_size;
              $collection_stats[$pid][$dsid]['datastream total size'] += $ds_size;
            }
            if ($op == 'all' || $op == 'dups') {
              if(user_access('delete fedora objects and datastreams')) {
                $object = islandora_object_load($object_result['solr_doc']['PID']);
                if (!empty($object[$dsid])) {
                  $ds = $object[$dsid];
                  if ($op == 'all' || ($op == 'dups' && count($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']) > 1)) {
                    while ($ds->count() > 1) {
                      $key = $ds->count() - 1;
                      if($timer) timer_start('ddds_delete');
                      unset($ds[$key]);
                      if($timer) timer_stop('ddds_delete');
                      $collection_stats['All Objects'][$dsid]['datastreams deleted count']++;
                      $collection_stats['All Objects']['datastreams deleted count']++;
                      $collection_stats[$pid]['datastreams deleted count']++;
                      $collection_stats['All Objects'][$dsid]['datastreams deleted total size'] += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
                      $collection_stats['All Objects']['datastreams deleted total size'] += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
                      $collection_stats[$pid][$dsid]['datastreams deleted total size'] += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
                    }
                  }
                  if ($op == 'all') {
                    drush_log($op, 'ok');
                    $object->purgeDatastream($dsid);
                  }
                }
              }
            }
          }

        }
      }

      if(count($qp->islandoraSolrResult['response']['objects']) > 0 && drupal_is_cli()) {
        $update_msg = $t("Objects checked: @object_count. Datastreams deleted: @deleted", array('@object_count' => $collection_stats['All Objects']['object count'], '@deleted' => $collection_stats['All Objects']['datastreams deleted count']));
        drush_log($update_msg, 'ok');
      }

      $qp->solrStart += $range;

    } while (!empty($qp->islandoraSolrResult['response']['objects'][0]));

    // Move all objects stats to last row of array.
    $all_objects_stats = $collection_stats['All Objects'];
    unset($collection_stats['All Objects']);
    $collection_stats['All Objects'] = $all_objects_stats;


    $rows = [['', 'DSID', 'Objects', 'Datastreams', 'File size', 'Dups file size', 'Dups deleted', 'Dups deleted size']];
    foreach($collection_stats as $id => $stats) {
      foreach($dsids as $dsid) {
        $rows[] = array(
          $id,
          $dsid,
          $stats[$dsid]['object count'],
          $stats[$dsid]['datastream count'],
          format_size($stats[$dsid]['datastream total size']),
          format_size($stats[$dsid]['datastream total size'] - $stats['first datastream total size']),
          $stats[$dsid]['datastreams deleted count'],
          format_size($stats[$dsid]['datastreams deleted total size']),
        );
      }
    }


    if(drupal_is_cli()) {
      drush_print_table($rows, TRUE);
    }
    else {
      $header = array_shift($rows);
      return theme('table', array('header' => $header, 'rows' => $rows));
    }
  }
}
