<?php

/**
 * Menu callback for admin/config/regional/translate/download
 */
function advanced_locale_download_form(){
	$form = array();

	$form['download'] = array(
		'#type' => 'submit',
		'#value' => t('Download files')
	);

	return $form;
}

/**
 * Submit callback of download files form
 * Download button
 */
function advanced_locale_download_form_submit($form, &$form_state){
	module_load_include('inc', 'advanced_locale_download');
	advanced_locale_download_search_files();
}
