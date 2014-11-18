(function ($) {
	Drupal.behaviors.advancedLocale = {
		attach: function(context){
			$('#restore_translations_table .type').click(function(e){

				var tr = $(this);
				var next = $(tr).next('tr');

				do{
					if($(next).hasClass('type')){
						next = false;
					}else if($(next).hasClass('name') || $(next).hasClass('lang')){
						$(next).toggle();
						next = $(next).next('tr');
					}else{
						next = $(next).next('tr');
					}
				}while(next.length);
			});
		}
	};
})(jQuery);