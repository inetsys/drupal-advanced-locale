<?php

function theme_advanced_locale_restore_form($vars){
	$output = '';
	$form = $vars['form'];

	$formatted = array();
	foreach(element_children($form['items']) as $i => $key){
		list($type, $name, $langcode) = explode('|', $key);
		$file = $form['items'][$key]['#return_value'];

		$formatted[$type][$name][$langcode] = $file;
	}

	$rows = array();
	$rows = array_merge($rows, _theme_advanced_locale_restore_form($formatted, $form['items'], ADVANCED_LOCALE_TYPE_CORE, t('Core')));
	$rows = array_merge($rows, _theme_advanced_locale_restore_form($formatted, $form['items'], ADVANCED_LOCALE_TYPE_CONTRIB, t('Contrib modules')));
	$rows = array_merge($rows, _theme_advanced_locale_restore_form($formatted, $form['items'], ADVANCED_LOCALE_TYPE_CUSTOM, t('Custom modules')));
	$rows = array_merge($rows, _theme_advanced_locale_restore_form($formatted, $form['items'], ADVANCED_LOCALE_TYPE_FEATURE, t('Features')));
	$rows = array_merge($rows, _theme_advanced_locale_restore_form($formatted, $form['items'], ADVANCED_LOCALE_TYPE_THEME, t('Themes')));

	$output = theme('table', array('rows' => $rows, 'header' => array(), 'attributes' => array('id' => 'restore_translations_table')));

	unset($form['#theme']);
	unset($form['items']);

	$output .= drupal_render($form);

	return $output;
}

function _theme_advanced_locale_restore_form($formatted, &$items, $type, $title){
	$rows = array();
	if(isset($formatted[$type])){
		$languages = language_list('language');

		$rows[$type] = array(
			'class' => array('type'),
			'data' => array(
				array(
					'data' => $title,
					'header' => true,
				)
			),
			'no_striping' => true
		);

		$count = array(
			'updated' => 0,
			'needs_review' => 0,
		);

		foreach($formatted[$type] as $name => $lang_list){
			if($type != ADVANCED_LOCALE_TYPE_CORE){
				$rows[$type.'|'.$name] = array(
					'class' => array('name'),
					'data' => array(
						array(
							'data' => $name,
							'header' => true,
							'colspan' => 3,
						)
					),
					'no_striping' => true
				);
			}

			foreach($lang_list as $langcode => $file){
				$key = $type.'|'.$name.'|'.$langcode;
				$field = $items[$key];
				$field['#title'] = t($languages[$langcode]->name);

				if(in_array('status_'.ADVANCED_LOCALE_NEEDS_REVIEW, $field['#attributes']['class'])){
					$status = '<span class="status_'.ADVANCED_LOCALE_NEEDS_REVIEW.'">'.t('Needs review').'</span>';
					$count['needs_review']++;
				}elseif(in_array('status_'.ADVANCED_LOCALE_UPDATED, $field['#attributes']['class'])){
					$status = '<span class="status_'.ADVANCED_LOCALE_UPDATED.'">'.t('Updated').'</span>';
					$count['updated']++;
				}else{
					$status = '';
				}

				$rows[$type.'|'.$name.'|'.$langcode] = array(
					'data' => array(
						drupal_render($field),
						$file,
						$status
					),
					'class' => array('lang')
				);
			}
		}

		$total = $count['updated'] + $count['needs_review'];
		$message = t('Files: !n_total (updated: !n_updated; to review: !n_review)', array('!n_total' => $total, '!n_updated' => $count['updated'], '!n_review' => $count['needs_review']));
		$rows[$type]['data'][] = array(
			'data' => '<span>'.$message.'</span>',
			'header' => true,
			'colspan' => 2
		);
		$rows[$type]['class'][] = $count['needs_review'] ? 'needs_review' : 'updated';
	}

	return $rows;
}
