<?php

function advanced_locale_get_data(){
	$languages = language_list('language');
	$translations_path = variable_get('advanced_locale_translations_path', conf_path().'/translations');

	$db_data = _advanced_locale_load_all(ADVANCED_LOCALE_NOT_FOUND);

	$types = _advanced_locale_get_types();

	foreach($types as $type){
		$tranlations_dir = $translations_path.'/'.$type;
		$regexp = '/(?<name>[a-z0-9_]+)-(?<version>7[a-z0-9\-\.]+)\.(?<code>[a-z\-]+)\.po$/i';

		$dh = @opendir($tranlations_dir);
		if($dh){
			while(($file = readdir($dh)) != false){
				if(in_array($file, array('.', '..', '.git'))){
					continue;
				}

				if(preg_match($regexp, $file, $m)){
					$code = $m['code'];
					$name = $m['name'];
					$version = $m['version'];

					if(!isset($languages[$code])){
						continue;
					}

					$check_file = $tranlations_dir.'/'.$file;

					if(isset($db_data[$type][$name][$code][$file])){
						$file_data = $db_data[$type][$name][$code][$file];

						$saved_last_update = $file_data->last_update;
						$saved_checksum = $file_data->checksum;

						$db_data[$type][$name][$code][$file]->status = _advanced_locale_get_status($check_file, $saved_last_update, $saved_checksum);
					}else{
						// New file
						$db_data[$type][$name][$code][$file] = (object)array(
							'status' => ADVANCED_LOCALE_NEEDS_REVIEW,
							'last_update' => 0
						);
						$data = (object)array(
							'type' => $type,
							'name' => $name,
							'language' => $code,
							'file' => $file,
							'checksum' => _advanced_locale_calc_checksum($check_file)
						);
						advanced_locale_insert($data);
					}
				}
			}

			closedir($dh);
		}
	}

	// Clean 404 files
	$clean_list = array();
	foreach($db_data as $type => $list){
		foreach($list as $name => $languages){
			foreach($languages as $code => $files){
				foreach($files as $file => $data){
					if($data->status == ADVANCED_LOCALE_NOT_FOUND){
						advanced_locale_delete((object)array(
							'type' => $type,
							'name' => $name,
							'language' => $code,
							'file' => $file
						));
					}else{
						$clean_list[$type][$name][$code][$file] = $data;
					}
				}
			}
		}
	}

	return $clean_list;
}

function advanced_locale_add_language($langcode, $default = false){
	include_once DRUPAL_ROOT.'/includes/iso.inc';
	$predefined = _locale_get_predefined_list();

	if(!isset($predefined[$langcode])){
		drupal_set_message(t('Language do not exists'), 'error');

		return false;
	}

	$existing = db_select('languages', 'l')
		->fields('l', array('language'))
		->condition('language', $langcode)
		->execute()
		->fetchAll();

	if(count($existing)){
		drupal_set_message(t('The language %language(%code) already exists.', array('%language' => $predefined[$langcode][1], '%code' => $langcode)), 'warning');

		if($default){
			variable_set('language_default',(object) array(
				'language' => $langcode,
				'name' => $predefined[$langcode][0],
				'native' => isset($predefined[$langcode][1]) ? $predefined[$langcode][1] : $predefined[$langcode][0],
				'direction' => isset($predefined[$langcode][2]) ? $predefined[$langcode][2] : LANGUAGE_LTR,
				'enabled' => 1,
				'plurals' => 0,
				'formula' => '',
				'domain' => '',
				'prefix' => $langcode,
				'weight' => 0,
				'javascript' => ''
			));
		}
	}else{
		include_once DRUPAL_ROOT.'/includes/locale.inc';
		locale_add_language($langcode, null, null, LANGUAGE_LTR, '', '', true, $default);
	}

	return true;
}

function advanced_locale_insert($data){
	if(!isset($data->type) || !isset($data->name) || !isset($data->language) || !isset($data->file)){
		return false;
	}

	db_insert('advanced_locale_translation_files')
		->fields(array(
			'type' => $data->type,
			'name' => $data->name,
			'language' => $data->language,
			'file' => $data->file,
			'checksum' => $data->checksum,
		))
		->execute();

	return true;
}
function advanced_locale_update($data){
	if(!isset($data->type) || !isset($data->name) || !isset($data->language) || !isset($data->file)){
		return false;
	}

	db_update('advanced_locale_translation_files')
		->fields(array(
			'last_update' => time(),
			'checksum' => $data->checksum
		))
		->condition('type', $data->type)
		->condition('name', $data->name)
		->condition('language', $data->language)
		->condition('file', $data->file)
		->execute();

	return true;
}
function advanced_locale_delete($data){
	if(!isset($data->type) || !isset($data->name) || !isset($data->language) || !isset($data->file)){
		return false;
	}

	db_delete('advanced_locale_translation_files')
		->condition('type', $data->type)
		->condition('name', $data->name)
		->condition('language', $data->language)
		->condition('file', $data->file)
		->execute();

	return true;
}

function _advanced_locale_load_all($status = ADVANCED_LOCALE_NEEDS_REVIEW){
	$data = array();
	$languages = language_list('language', true);

	$list = db_select('advanced_locale_translation_files', 'f')
		->fields('f')
		->condition('language', array_keys($languages), 'IN')
		->orderBy('type', 'ASC')
		->orderBy('name', 'ASC')
		->orderBy('language', 'ASC')
		->execute()
		->fetchAll();

	if($list){
		foreach($list as $row){
			$type = $row->type;
			$name = $row->name;
			$code = $row->language;
			$file = $row->file;

			$data[$type][$name][$code][$file] = (object)array(
				'status' => $status,
				'last_update' => $row->last_update,
				'checksum' => $row->checksum
			);
		}
	}

	return $data;
}
function _advanced_locale_calc_checksum($file){
	$checksum = '';
	$fd = fopen($file, 'r');
	if($fd){
		$content = '';
		while(!feof($fd)){
			$content .= fread($fd, 1024);
		}

		// Clean BOM
		$bom = "\xEF\xBB\xBF";
		if(strpos($content, $bom) === 0){
			$content = str_replace($bom, '', $content);
		}

		$checksum = sha1($content);
	}

	return $checksum;
}
function _advanced_locale_get_status($file, $last_update, $checksum){
	$mtime = filemtime($file);

	return !$last_update ? ADVANCED_LOCALE_NEEDS_REVIEW :(
		$mtime < $last_update ? ADVANCED_LOCALE_UPDATED :(
			_advanced_locale_calc_checksum($file) == $checksum ? ADVANCED_LOCALE_UPDATED : ADVANCED_LOCALE_NEEDS_REVIEW
		)
	);
}

function advanced_locale_restore_all_translations($mode = LOCALE_IMPORT_KEEP, $langcode = null, $force = false, $batch_process = true, $batch_return = false){
	$db_data = advanced_locale_get_data();
	$list = array();
	foreach($db_data as $type => $type_list){
		foreach($type_list as $name => $languages){
			foreach($languages as $code => $files){
				if($langcode && $langcode != $code){
					continue;
				}
				foreach($files as $file => $data){
					if($force || $data->status == ADVANCED_LOCALE_NEEDS_REVIEW){
						$list[] = (object)array(
							'name' => $name,
							'type' => $type,
							'code' => $code,
							'file' => $file
						);
					}
				}
			}
		}
	}

	return advanced_locale_restore_translations($mode, $list, $batch_process, $batch_return);
}
function advanced_locale_restore_translations($mode, $list, $batch_process = true, $batch_return = false, $destination = 'admin/config/regional/translate/restore'){
	$action = '_advanced_locale_restore_translations_batch';
	$finished = '_advanced_locale_restore_translations_batch_finished';
	$operations = array();

	foreach($list as $data){
		$params = array(
			$mode,
			$data->type,
			$data->name,
			$data->code,
			$data->file
		);
		$operations[] = array($action, $params);
	}

	if($operations){
		$batch = array(
			'operations' => $operations,
			'finished' => $finished,
			'title' => t('Restore translations'),
			'init_message' => t('Process is starting...'),
			'progress_message' => t('Processed @current out of @total.'),
			'error_message' => t('Process has encountered an error.'),
			'file' => drupal_get_path('module', 'advanced_locale').'/advanced_locale.inc',
		);
		if($batch_return){
			return $batch;
		}else{
			batch_set($batch);

			if($batch_process){
				batch_process($destination);
			}
		}
	}
}
function _advanced_locale_restore_translations_batch($mode, $type, $name, $code, $file, &$context){
	if(advanced_locale_restore_translation($mode, $type, $name, $code, $file)){
		$context['results']['success'][] = array('name' => $name, 'code' => $code);
		$context['message'] = t('Updated %code translations for %name', array('%name' => $name, '%code' => $code));
	}else{
		$context['results']['error'][] = array('name' => $name, 'code' => $code);
	}
}
function _advanced_locale_restore_translations_batch_finished($success, $results, $operations){
	if($success){
		drupal_set_message(t('%count translations updated', array('%count' => isset($results['success']) ? count($results['success']) : 0)), 'status');
		if(isset($results['error'])){
			drupal_set_message(t('%count translations could not be updated', array('%count' => count($results['error']))), 'error');
		}
	}else{
		$error_operation = reset($operations);
		drupal_set_message(t('An error occurred while processing %operation with arguments: !arguments', array('%operation' => $error_operation[0], '!arguments' => print_r($error_operation[0], TRUE))), 'error');
	}
}
function advanced_locale_restore_translation($mode, $type, $name, $code, $file){
	include_once DRUPAL_ROOT.'/includes/locale.inc';

	$mode = in_array($mode, array(LOCALE_IMPORT_OVERWRITE, LOCALE_IMPORT_KEEP)) ? $mode : LOCALE_IMPORT_KEEP;
	switch($type){
		case ADVANCED_LOCALE_TYPE_CUSTOM:
		case ADVANCED_LOCALE_TYPE_CONTRIB:
		case ADVANCED_LOCALE_TYPE_FEATURE:
		case ADVANCED_LOCALE_TYPE_CORE:
		case ADVANCED_LOCALE_TYPE_THEME:
			$filepath = variable_get('advanced_locale_translations_path', conf_path().'/translations').'/'.$type.'/'.$file;
			break;
		default:
			$filepath = null;
	}
	if($filepath && file_exists($filepath)){
		$file_object = new stdClass();
		$file_object->uri = $filepath;
		$file_object->filename = basename($filepath);

		if(_locale_import_po($file_object, $code, $mode, 'default')){
			// Update DB
			$data = (object)array(
				'type' => $type,
				'name' => $name,
				'language' => $code,
				'file' => $file,
				'checksum' => _advanced_locale_calc_checksum($filepath)
			);
			advanced_locale_update($data);
		}else{
			return false;
		}
	}

	return true;
}
