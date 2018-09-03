/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

	(function($){
		
		var $desc = $(".describe td").eq(1);
		var $temp = $(".describe ul");
		
		$(".type :radio").change(function(){
			
			var $index = $(this).index();
			
			$index == 0 ? $desc.prepend($temp) : $temp.detach();
			
			});
		
		$(".sbmt span").click(function(){
			
			var path = $(".path").val();
			var data = $("#fileinfo").serialize();
			
			var re1 = /describe=[^&]/;
			var re2 = /tag=[1-6]/;
			var re3 = /path=\w{1,}(.png|.gif|.jpg)/i;
			
			
			console.log(data.match(re3));
			
			//判断图片是否合规
			if(path.match(re3)){
				
				
				}
			
			//判断信息是否有用
			if(data.match(re1) || data.match(re2) || path.match(re3) || $("input[name = 'url']").val()){
				$(".sbmt i").text("感谢您的反馈,我们会尽快处理！").attr("data-info","s");
				}else{
					$(".sbmt i").text("请填写内容后提交！").attr("data-info","f");
					}
				console.log(data);
				
				/*$.ajax({
					type    : "post",
					url     : "http://www.baidu.com/s?kw=",
					data    : $("#fileinfo").serialize(),
					dataType: "html",
					success : function(){
								$(".sbmt i").text(data);
							  },
					error   : function(err){
								$(".sbmt").text(err);
							  }
				});*/
				
			});
			
		
	})(jQuery);