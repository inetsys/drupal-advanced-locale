<?php

function advanced_locale_download_search_files($langcode = false, $is_update = false, $destination = 'admin/config/regional/translate/download'){
	$operations = array();

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
		drupal_set_message('Empty list of languages to update', 'error');
		return;
	}

	$operations = array();

	// Contrib modules
	$type = 'module';
	$list = db_select('system', 's')
		->fields('s', array('name', 'info'))
		->condition('status', 1)
		->condition('type', 'module')
		->condition('filename', '%/contrib/%', 'like')
		->execute()
		->fetchAllAssoc('name');
	foreach($list as $name => $data){
		$info = unserialize($data->info);
		$version = $info['version'];
		foreach($langcodes as $langcode){
			$operations[] = array('_advanced_locale_download_download_files_batch', array($name, $version, $langcode, $type));
		}
	}

	// Core
	$name = 'drupal';
	$type = 'core';
	$version = VERSION;
	foreach($langcodes as $langcode){
		$operations[] = array('_advanced_locale_download_download_files_batch', array($name, $version, $langcode, $type));
	}

	if($operations){
		$batch = array(
			'operations' => $operations,
			'finished' => '_advanced_locale_download_download_files_batch_finished',
			'title' => t('Update Drupal translations'),
			'init_message' => t('Process is starting...'),
			'progress_message' => t('Processed @current out of @total.'),
			'error_message' => t('Process has encountered an error.'),
			'file' => drupal_get_path('module', 'advanced_locale_download').'/advanced_locale_download.inc',
		);
		batch_set($batch);

		if(!$is_update){
			batch_process($destination);
		}
	}
}
function _advanced_locale_download_download_files_batch($name, $version, $langcode, $type, &$context){
	$message = t('Downloading %name in %langcode...', array('%name' => $name, '%langcode' => $langcode));

	$translations_folder = rtrim(variable_get('advanced_locale_download_translations_path', conf_path().'/translations'), '/');
	$filename = $name.'-'.$version.'.'.$langcode.'.po';
	switch($type){
		case 'module':
			$output_file = $translations_folder.'/contrib/'.$filename;
			break;

		case 'core':
			$output_file = $translations_folder.'/core/'.$filename;
			break;

		default:
			$output_file = $translations_folder.'/'.$filename;
			break;
	}

	if(file_exists($output_file)){
		$message .= ' Skipped';
		$context['results']['skipped'][] = $name;
	}else{
		$url = 'http://ftp.drupal.org/files/translations/7.x/'.$name.'/'.$filename;

		$ch = curl_init();

		curl_setopt($ch, CURLOPT_URL, $url);
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

		$content = curl_exec($ch);
		$http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);

		curl_close($ch);


		if($http_code == 200){
			$message .= ' Ok';
			$context['results']['downloaded'][] = $name;

			$fh = fopen($output_file, 'w');
			if($fh !== false){
				fwrite($fh, $content);
				fclose($fh);
			}
		}elseif($http_code == 404){
			$message .= 'Not found';
			$context['results']['not_found'][] = $name;
		}else{
			$message .= ' Error';
			$context['results']['error'][] = $name;
		}
	}

	$context['message'] = $message;
}
function _advanced_locale_download_download_files_batch_finished($success, $results, $operations){
	if($success){
		if(isset($results['downloaded']) && $results['downloaded']){
			drupal_set_message(t('%count PO files downloaded successfully from drupal.org', array('%count' => count($results['downloaded']))), 'status');
		}
		if(isset($results['skipped']) && $results['skipped']){
			drupal_set_message(t('%count files had been already downloaded', array('%count' => count($results['skipped']))), 'warning');
		}
		if(isset($results['not_found']) && $results['not_found']){
			drupal_set_message(t('%count files had not been found in drupal.org', array('%count' => count($results['not_found']))), 'warning');
		}
		if(isset($results['error']) && $results['error']){
			drupal_set_message(t('%count PO files could not been downloaded from drupal.org', array('%count' => count($results['error']))), 'error');
		}
	}else{
		$error_operation = reset($operations);
		drupal_set_message(t('An error occurred while processing %operation with arguments : !arguments', array('%operation' => $error_operation[0], '!arguments' => print_r($error_operation[0], TRUE))), 'error');
	}
}

