<?php

/**
 * Implements hook_drush_help
 */
function advanced_locale_drush_help($command) {
	switch($command){
		case 'drush:restore-translations':
			return dt('Restore translations from local PO files');
	}
}

/**
 * Implements hook_drush_command
 */
function advanced_locale_drush_command() {
	$items = array();

	$items['restore-translations'] = array(
		'description' => dt('Restore translations from local PO files'),
		'arguments' => array(
			'langcode' => dt('Language to restore translations'),
		),
		'options' => array(
			'force-all' => array(
				'description' => dt('Restore all files without check their status'),
				'value' => 'none'
			),
			'mode' => array(
				'description' => dt('keep/override'),
				'value' => 'required'
			),
		),
		'aliases' => array('rt'),
	);

	return $items;
}

/**
 * Callback function for drush restore-translations
 *
 * @param $arg1
 *   An optional argument
 */
function drush_advanced_locale_restore_translations($langcode = NULL) {
	$force_all = (bool) drush_get_option('force-all');

	$mode = drush_get_option('mode');
	switch($mode){
		case 'keep':
		default:
			$mode = LOCALE_IMPORT_KEEP;
			break;
		case 'override':
			$mode = LOCALE_IMPORT_OVERWRITE;
			break;
	}

	module_load_include('inc', 'advanced_locale');
	$batch = advanced_locale_restore_all_translations($mode, $langcode, $force_all, false, true);
	if($batch){
		//Get the batch process all ready!
		batch_set($batch);
		$batch =& batch_get();

		//Because we are doing this on the back-end, we set progressive to false.
		$batch['progressive'] = FALSE;

		//Start processing the batch operations.
		drush_backend_batch_process();
	}

	//log to the command line with an OK status
	drush_log('All done', 'ok');
}

