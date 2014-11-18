<?php

function advanced_locale_extract_search_files($langcode = false, $is_update = false, $destination = 'admin/config/regional/translate/extract'){
	$types = _advanced_locale_get_types();

	$langcodes = array();
	$languages = language_list('language');
	if(isset($languages['en'])){
		unset($languages['en']);
	}

	if($langcode){
		if(isset($languages[$langcode])){
			$langcodes[] = $langcode;
		}
	}else{
		$langcodes = array_keys($languages);
	}

	if(empty($langcodes)){
		drupal_set_message(t('Empty list of languages to export'), 'error');
		return;
	}

	$action = '_advanced_locale_extract_search_files_batch';
	$finished = '_advanced_locale_extract_search_files_batch_finished';
	$operations = array();
	foreach($types as $type){
		foreach(_advanced_locale_extract_get_system_dirs($type) as $name => $data){
			$directory = $data['directory'];
			$version = $data['version'];

			foreach($langcodes as $langcode){
				$params = array($type, $name, $version, $directory, $langcode);
				$operations[] = array($action, $params);
			}
		}
	}

	if($operations){
		$batch = array(
			'operations' => $operations,
			'finished' => $finished,
			'title' => t('Extract translatable strings'),
			'init_message' => t('Process is starting...'),
			'progress_message' => t('Processed @current out of @total.'),
			'error_message' => t('Process has encountered an error.'),
			'file' => drupal_get_path('module', 'advanced_locale_extract').'/advanced_locale_extract.inc',
		);
		batch_set($batch);

		if(!$is_update){
			batch_process($destination);
		}
	}
}

function _advanced_locale_extract_search_files_batch($type, $name, $version, $directory, $langcode, &$context){
	if(advanced_locale_extract_extract_strings($type, $name, $version, $directory, $langcode)){
		$context['results']['success'][] = array('name' => $name, 'code' => $code);
		$context['message'] = t('Translatable strings extracted from %directory of %name in %langcode', array('%directory' => $dir, '%name' => $name, '%langcode' => $langcode));
	}else{
		$context['results']['error'][] = array('name' => $name, 'code' => $code);
	}
}

function _advanced_locale_extract_search_files_batch_finished($success, $results, $operations){
	if($success){
		drupal_set_message(t('%count files generated', array('%count' => isset($results['success']) ? count($results['success']) : 0)), 'status');
		if(isset($results['error'])){
			drupal_set_message(t('%count files could not be created', array('%count' => count($results['error']))), 'error');
		}
	}else{
		$error_operation = reset($operations);
		drupal_set_message(t('An error occurred while processing %operation with arguments: !arguments', array('%operation' => $error_operation[0], '!arguments' => print_r($error_operation[0], TRUE))), 'error');
	}
}

function _advanced_locale_extract_get_system_dirs($type){
	$directories = array();

	$query = db_select('system', 's')
		->fields('s', array(
			'name',
			'filename',
			'info'
		));

	switch($type){
		case ADVANCED_LOCALE_TYPE_CUSTOM:
			$query
				->condition('status', 1)
				->condition('filename', '%/modules/custom/%', 'like');
			break;

		case ADVANCED_LOCALE_TYPE_FEATURE:
			$query
				->condition('status', 1)
				->condition('filename', '%/modules/features/%', 'like');
			break;

		case ADVANCED_LOCALE_TYPE_CONTRIB:
			$query
				->condition('status', 1)
				->condition('filename', '%/modules/contrib/%', 'like');
			break;

		case ADVANCED_LOCALE_TYPE_THEME:
			$query
				->condition('type', 'theme')
				->condition('filename', '%/themes/custom/%', 'like');
			break;

		default:
			return array();
	}

	$list = $query->execute()->fetchAll();

	foreach($list as $row){
		$info = unserialize($row->info);
		$directory = dirname($row->filename);
		$directories[$row->name] = array(
			'name' => $row->name,
			'directory' => $directory,
			'version' => $info['version'],
		);
	}

	return $directories;
}

function advanced_locale_extract_extract_strings($type, $name, $version, $directory, $langcode){
	module_load_include('inc', 'potx');

	global $_potx_tokens, $_potx_lookup, $_potx_store, $_potx_versions, $_potx_strings, $_potx_install;;
	$_potx_tokens = $_potx_lookup = $_potx_store = $_potx_versions = $_potx_strings = $_potx_install = array();

	$translations_path = variable_get('advanced_locale_translations_path', conf_path().'/translations');
	$dir = $translations_path.'/'.$type;
	$output = $dir.'/'.$name.'-'.$version.'.'.$langcode.'.po';

	$strings = array();

	$files = _potx_explore_dir($directory.'/');
	foreach($files as $file){
		_potx_process_file($file);
	}
	_potx_build_files(POTX_STRING_RUNTIME, POTX_BUILD_SINGLE, $name, '_potx_save_string', '_potx_save_version', '_potx_get_header', $langcode, $langcode);

	return _advanced_locale_write_files($output);
}

function _advanced_locale_write_files($filename){
	global $_potx_store;

	$header = '';
	$output = '';
	// Generate file lists and output files.
	if(is_array($_potx_store)) {
		foreach($_potx_store as $file => $contents){
			// Build replacement for file listing.
			if(count($contents['sources']) > 1) {
				$filelist = "Generated from files:\n#  ".join("\n#  ", $contents['sources']);
			}elseif(count($contents['sources']) == 1) {
				$filelist = "Generated from file: ".join('', $contents['sources']);
			}else{
				$filelist = 'No version information was available in the source files.';
			}
			$header = $header ? $header : str_replace('--VERSIONS--', $filelist, $contents['header']);
			$output .= "\n".$contents['strings'];
		}

	}

	$_potx_store = array();

	$fp = fopen($filename, 'w');
	if($fp !== false){
		fwrite($fp, $header.$output);
		fclose($fp);
		drupal_set_message(t('File %filename created', array('%filename' => $filename)));

		return true;
	}

	return false;
}
