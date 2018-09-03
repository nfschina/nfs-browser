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

(function() {
  var range = {
    x: 0,
    y: 0
  };
  var lastPos = {
    x: 0,
    y: 0,
    x1: 0,
    y1: 0
  };
  var tarPos = {
    x: 0,
    y: 0,
    x1: 0,
    y1: 0
  };
  var theDiv = null,
    ismove = false,
    choose = false;
  var theDivId = 0,
    theDivWidth = 230,
    theDivHeight = 154;
  var tarDiv = null,
    tarFirst, tempDiv;
  var initPos = {
    x: 0,
    y: 0
  };
  $(".horizontal-li").each(function(i) {
    $(this).mousedown(function(event) {
      if (event.which == 1) {
        choose = true;
        theDiv = $(this);
        initPos.x = theDiv.offset().left;
        initPos.y = theDiv.offset().top;
        range.x = event.pageX;
        range.y = event.pageY;
        theDivId = theDiv.index();
        theDiv.attr("class", "horizontal-li-dash");
        theDiv.css({
          left: initPos.x + "px",
          top: initPos.y + "px"
        });
        $("<li class='dash'></li>").insertBefore(theDiv);
        tempDiv = $(".dash");
      }

    })
  });
  $(document).mouseup(function(event) {

  	if(event.target.nodeName != "INPUT"){
	    if (!choose || event.which == 2) {
	      return false;
	    }
	    if (!ismove) {
	      theDiv.attr("class", "horizontal-li");
	      tempDiv.remove();
	      choose = false;
	      ismove = false;
	      return false;
	    }
	    theDiv.insertBefore(tempDiv);
	    theDiv.removeClass();
	    theDiv.attr("class", "horizontal-li");
	    tempDiv.remove();
	    ismove = false;
	    choose = false;

	    ntp.resetWebsiteList();
  	}
  }).mousemove(function(event) {
    if (choose) {
	    ismove = true;
	    ismove = true;
	    lastPos.x = event.pageX - range.x + initPos.x;
	    lastPos.y = event.pageY - range.y + initPos.y;
	    theDiv.css({
	      left: lastPos.x + "px",
	      top: lastPos.y + "px"
	    });
	    var $main = $(".horizontal-li");
	    theDiv.addClass("frozen");
	    $main.each(function(i) {
	      tarDiv = $(this);
	      tarPos.x = tarDiv.offset().left;
	      tarPos.y = tarDiv.offset().top;
	      tarPos.x1 = tarPos.x + theDivWidth;
	      tarPos.y1 = tarPos.y + theDivHeight;
	      if (event.pageX >= tarPos.x && event.pageX <= tarPos.x1 && event.pageY >= tarPos.y && event.pageY <= tarPos.y1) {
	        if (theDivId > i) {
	          tempDiv.insertBefore(tarDiv);
	          theDivId = i;
	        } else {
	          tempDiv.insertAfter(tarDiv);
	          theDivId = i + 1;
	        }
	      }
	    });
  	}
  });
})();
