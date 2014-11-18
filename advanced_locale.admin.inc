<?php

function advanced_locale_restore_translations_form($form, $form_state){
	// DB data
	module_load_include('inc', 'advanced_locale');
	$db_data = array();
	$db_data = advanced_locale_get_data();

	// Form
	$form = array();

	$form['#attached']['css'] = array(
		drupal_get_path('module', 'advanced_locale') . '/advanced_locale.css',
	);
	$form['#attached']['js'] = array(
		drupal_get_path('module', 'advanced_locale') . '/advanced_locale.js',
	);

	$form['items'] = array(
		'#tree' => true,
	);
	foreach($db_data as $type => $list){
		foreach($list as $name => $languages){
			foreach($languages as $code => $files){
				foreach($files as $file => $data){
					$form['items'][$type.'|'.$name.'|'.$code] = array(
						'#type' => 'checkbox',
						'#title' => $file,
						'#return_value' => $file,
						'#attributes' => array(
							'class' => array('status_'.$data->status)
						)
					);
				}
			}
		}
	}

	// Mode
	include_once './includes/locale.inc';

	$form['mode'] = array(
		'#type' => 'radios',
		'#required' => true,
		'#title' => t('Restoration mode'),
		'#options' => array(
			LOCALE_IMPORT_OVERWRITE => t('Translation updates replace existing ones, new ones are added'),
			LOCALE_IMPORT_KEEP => t('All existing translations are kept, only new translations are added.')
		),
		'#default_value' => LOCALE_IMPORT_KEEP
	);

	$form['buttons']['checked'] = array(
		'#type' => 'submit',
		'#value' => t('Restore checked translations'),
		'#weight' => '10',
		'#submit' => array('advanced_locale_restore_form_checked_submit'),
		'#weight' => 1
	);
	$form['buttons']['full'] = array(
		'#type' => 'submit',
		'#value' => t('Restore all translations'),
		'#weight' => '10',
		'#submit' => array('advanced_locale_restore_form_full_submit'),
		'#weight' => 2
	);

	// Output
	$form['#theme'] = 'advanced_locale_restore_form';

	return $form;
}

function advanced_locale_restore_form_checked_submit($form, &$form_state){
	module_load_include('inc', 'advanced_locale');

	$mode = $form_state['values']['mode'];
	$items = $form_state['values']['items'];

	$list = array();

	foreach($items as $key => $file){
		if($file){
			list($type, $name, $code) = explode('|', $key);
			$list[$key] = (object)array(
				'name' => $name,
				'type' => $type,
				'code' => $code,
				'file' => $file
			);
		}
	}

	advanced_locale_restore_translations($mode, $list);
}

function advanced_locale_restore_form_full_submit($form, &$form_state){
	module_load_include('inc', 'advanced_locale');

	$mode = $form_state['values']['mode'];
	$db_data = advanced_locale_get_data();
	$list = array();

	foreach($db_data as $type => $type_list){
		foreach($type_list as $name => $languages){
			foreach($languages as $code => $files){
				foreach($files as $file => $data){
					if($data->status == ADVANCED_LOCALE_NEEDS_REVIEW){
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

	advanced_locale_restore_translations($mode, $list);
}