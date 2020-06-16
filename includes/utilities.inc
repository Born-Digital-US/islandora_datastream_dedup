<?php

function _datastream_dedup($op, $dsid = 'OBJ', $options = array()) {

  $mode = $op['mode']; // preview or nuke
  $scope = $op['scope']; // dups or all
  $leave = $scope === 'all' ? 0 : 1; // leave 1 if dups, otherwise leave zero

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
        $data = str_getcsv($value);
        $data = array_map('trim',$data);
        $mimetypes = array_filter($data);
        $mimetype_query_ors = array();
        foreach($mimetypes as $mimetype) {
          foreach($dsids as $dsid) {
            $mimetype_query_ors[] = 'fedora_datastream_latest_' . $dsid . '_MIMETYPE_mt:"' . $mimetype . '"';
          }
        }
        $query[] = '(' . implode(' OR ', $mimetype_query_ors) . ')';
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
      case 'cm': {
        $data = str_getcsv($value);
        $data = array_map('trim',$data);
        $content_models = array_filter($data);
        dpm(compact('content_models'));

      }
    }
  }

  $pids = array_unique($pids);



  if(!empty($pids)) {
    $query_pids = array();
    foreach($pids as $i => $pid) {
      $query_pids[] = 'RELS_EXT_isMemberOfCollection_uri_s:"info:fedora/' . $pid . '"';
    }
    $query[] = $exclude . '(' . implode(' OR ',$query_pids) . ')';
  }

  if(!empty($content_models)) {
    $query_cms = array();
    foreach($content_models as $i => $cm) {
      $query_cms[] = 'RELS_EXT_hasModel_uri_ss:"info:fedora/' . $cm . '"';
    }
    $query[] =  '(' . implode(' OR ',$query_cms) . ')';
  }


  $membership_fields = array(
    'RELS_EXT_isMemberOfCollection_uri_s',
    'RELS_EXT_isConstituentOf_uri_s',
    'RELS_EXT_isMemberOf_uri_s',
    'RELS_EXT_isPageOf_uri_s',
  );
  $fl = array(
    'PID',
    'fgs_label_s',
  );
  $ds_size_fields = array();
  foreach($dsids as $dsid_value) {
    $ds_size_fields[] = 'fedora_datastream_version_' . $dsid_value . '_SIZE_ms';
  }
  $ds_size_filters = array();
  foreach($ds_size_fields as $ds_size_field) {
    $ds_size_filters[] = $ds_size_field . " : [ * TO * ]";
  }
  $query[] = "( " . implode(" OR ", $ds_size_filters) . " )";
  $fl = array_merge($fl, $membership_fields);
  $fl = array_merge($fl, $ds_size_fields);

  $query = implode(' AND ', $query);
  $qp = new IslandoraSolrQueryProcessor();

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
        '@dsid' => implode(',', $dsids),
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
        $pid = "Not a member of any object";
        foreach($membership_fields as $membership_field) {
          if(!empty($object_result['solr_doc'][$membership_field]) && !empty(array_intersect_key($object_result['solr_doc'], array_flip($ds_size_fields)))) {
            $pid = str_replace('info:fedora/', '', $object_result['solr_doc'][$membership_field]);
            break;
          }
        }
        foreach($dsids as $dsid) {
          if(!isset($collection_stats[$pid][$dsid])) {
            $collection_stats[$pid][$dsid] = $collection_stats_record_template;
          }
          if (!empty($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'])) {

            $collection_stats['All Objects'][$dsid]['object count']++;
            $collection_stats[$pid][$dsid]['object count']++;
            $first_datastream_size = reset($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']);
            $collection_stats['All Objects'][$dsid]['first datastream total size'] += $first_datastream_size;
            $collection_stats[$pid][$dsid]['first datastream total size'] += $first_datastream_size;
            foreach ($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'] as $ds_size) {
              $collection_stats['All Objects'][$dsid]['datastream count']++;
              $collection_stats[$pid][$dsid]['datastream count']++;
              $collection_stats['All Objects'][$dsid]['datastream total size'] += $ds_size;
              $collection_stats[$pid][$dsid]['datastream total size'] += $ds_size;
            }
            $ds_count = count($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']);
            $nuke = $mode == 'nuke' && user_access('delete fedora objects and datastreams');
            $object = islandora_object_load($object_result['solr_doc']['PID']);
            $ds = $object[$dsid] ?? NULL;

            if ($ds_count > $leave ) {
                for($key = $ds_count -1; $key >= $leave; $key--) {
                  if ($nuke && $ds) {
                    if($timer) timer_start('ddds_delete');
                      unset($ds[$key]);
                    if($timer) timer_stop('ddds_delete');
                  }
                  $collection_stats['All Objects'][$dsid]['datastreams deleted count']++;
                  $collection_stats[$pid][$dsid]['datastreams deleted count']++;
                  $collection_stats['All Objects'][$dsid]['datastreams deleted total size'] += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
                  $collection_stats[$pid][$dsid]['datastreams deleted total size'] += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
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

    $preview_label = $mode == 'nuke' ? "" : "Preview ";
    $rows = [['', 'DSID', 'Objects', 'Datastreams', 'Total File Size', 'Dups File Size', $preview_label . 'Datastreams Deleted', $preview_label . 'Deleted Datastream Size']];
    foreach($collection_stats as $id => $stats) {
      // Insert visual break in table before All Objects summary section.
      if($id == 'All Objects') {
        $rows['hr'] = ['---', '---', '---', '---', '---', '---', '---' . '---', '---' . '---'];
      }
      foreach($dsids as $dsid) {
        $rows[] = array(
          $id,
          $dsid,
          $stats[$dsid]['object count'],
          $stats[$dsid]['datastream count'],
          format_size($stats[$dsid]['datastream total size']),
          format_size($stats[$dsid]['datastream total size'] - $stats[$dsid]['first datastream total size']),
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
  else {
    $msg = $t("No objects were found matching these filter criteria.", array());
    drush_log($msg, 'ok');

  }
}
