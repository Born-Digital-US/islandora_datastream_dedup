<?php

function _datastream_dedup($mimetype = "image/tiff", $op = 'preview', $dsid = 'OBJ', $timer = FALSE) {


  $qp = new IslandoraSolrQueryProcessor();
  $query = 'fedora_datastream_latest_' . $dsid . '_MIMETYPE_mt:"' . $mimetype . '"';
  $qp->buildQuery($query);
  $qp->solrParams['fl'] = 'PID, fgs_label_s, fedora_datastream_version_' . $dsid . '_SIZE_ms';
  $qp->solrStart = 0;
  $qp->solrLimit = 1;
  timer_start('ddds_execute_solr_query');
  $qp->executeQuery(FALSE);
  timer_stop('ddds_execute_solr_query');
  $object_count = 0;
  $datastream_count = 0;
  $first_datastream_total_size = 0;
  $datastream_total_size = 0;
  $datastream_deleted_count = 0;
  $datastreams_deleted_total_size = 0;
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
      $msg = $t("Preparing to inspect @num objects for duplicate @dsid datastreams matching the '@type' mimetype.", array(
        '@num' => $qp->islandoraSolrResult['response']['numFound'],
        '@dsid' => $dsid,
        '@type' => $mimetype,
      ));
      drush_log($msg, 'ok');
    }
    do {
      $qp->solrLimit = $range;
      timer_start('ddds_execute_solr_query');
      $qp->executeQuery(FALSE);
      timer_stop('ddds_execute_solr_query');
      foreach ($qp->islandoraSolrResult['response']['objects'] as $object_result) {
        if (!empty($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'])) {
          $object_count++;
          $first_datastream_total_size += reset($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']);
          foreach ($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'] as $ds_size) {
            $datastream_count++;
            $datastream_total_size += $ds_size;
          }
          if ($op == 'nuke' && count($object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms']) > 1) {
            if(user_access('delete fedora objects and datastreams')) {
              $object = islandora_object_load($object_result['solr_doc']['PID']);
              if (!empty($object[$dsid])) {
                $ds = $object[$dsid];
                while ($ds->count() > 1) {
                  $key = $ds->count() - 1;
                  timer_start('ddds_delete');
                  unset($ds[$key]);
                  timer_stop('ddds_delete');
                  $datastream_deleted_count++;
                  $datastreams_deleted_total_size += $object_result['solr_doc']['fedora_datastream_version_' . $dsid . '_SIZE_ms'][$key];
                }
              }
            }
          }
        }
      }

      if(count($qp->islandoraSolrResult['response']['objects']) > 0 && drupal_is_cli()) {
        $update_msg = $t("Objects checked: @object_count. Datastreams deleted: @deleted", array('@object_count' => $object_count, '@deleted' => $datastream_deleted_count));
        drush_log($update_msg, 'ok');
      }

      $qp->solrStart += $range;

    } while (!empty($qp->islandoraSolrResult['response']['objects'][0]));

    $report = $t("Processing complete.
@object_count objects found.
@datastream_count @dsid datastreams found.
Total initial file size of @dsid datastreams = @datastream_total_size.
Total size of duplicate @dsid datastreams = @dup_datastream_size.
Number of duplicate @dsid datastreams deleted = @datastream_deleted_count.
Total size of duplicate @dsid datastreams deleted = @datastreams_deleted_total_size", array(
      '@object_count' => $object_count,
      '@datastream_count' => $datastream_count,
      '@datastream_total_size' => format_size($datastream_total_size),
      '@dup_datastream_size' => format_size($datastream_total_size - $first_datastream_total_size),
      '@datastream_deleted_count' => $datastream_deleted_count,
      '@datastreams_deleted_total_size' => format_size($datastreams_deleted_total_size),
      '@dsid' => $dsid,
    ));

    if(drupal_is_cli()) {
      drush_print("\n" . $report);

      $rows = array(
        array($query, 'count or bytes', 'formatted' ),
        array('object count', $object_count, ''),
        array('datastream count', $datastream_count,''),
        array('total datastream file size', $datastream_total_size, format_size($datastream_total_size)),
        array('size of duplicate datastream files', $datastream_total_size - $first_datastream_total_size, format_size($datastream_total_size - $first_datastream_total_size)),
      );
      drush_print_table($rows, TRUE);
    }
    else {
      drupal_set_message($report);
    }
  }
}