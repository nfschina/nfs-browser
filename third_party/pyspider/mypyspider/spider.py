#!/usr/bin/env python
# -*- encoding: utf-8 -*-
# Created on 2018-01-02 13:48:16
# Project: test

from pyspider.libs.base_handler import *


class Handler(BaseHandler):
    crawl_config = {
    }

    @every(minutes=24 * 60)
    def on_start(self):
        self.crawl('http://www.hao123.com', callback=self.index_page)

    @config(age=10 * 24 * 60 * 60)
    def index_page(self, response):
        self.crawl('http://www.hao123.com', callback=self.detail_page, fetch_type='js')
        for each in response.doc('a[href^="http"]').items():
            self.crawl(each.attr.href, callback=self.detail_page, fetch_type='js')

    @config(priority=2)
    def detail_page(self, response):
        stringaa = response.text.encode("utf-8")  
        ret = stringaa.find('ActiveXObject') != -1
        if ret:
          return {
               "url": response.url,
               "string":stringaa,
               "text":response.doc.html(),
                "title": response.doc('title').text(),
           }




#!/usr/bin/env python
# -*- encoding: utf-8 -*-
# Created on 2018-01-02 13:48:16
# Project: test

from pyspider.libs.base_handler import *


class Handler(BaseHandler):
    crawl_config = {
    }

    @every(minutes=24 * 60)
    def on_start(self):
        self.crawl('http://www.baidu.com', callback=self.index_page)

    @config(age=-1)
    def index_page(self, response):
        self.crawl('http://www.baidu.com', callback=self.detail_page3, fetch_type='js')
        for each in response.doc('a[href^="http"]').items():
            #self.crawl(each.attr.href, callback=self.detail_page3, fetch_type='js')
            self.crawl(each.attr.href, callback=self.detail_page3, fetch_type='js')

    @config(priority=2)
    def detail_page1(self, response):
        for each in response.doc('a[href^="http"]').items():
            #self.crawl(each.attr.href, callback=self.detail_page3, fetch_type='js')
            self.crawl(each.attr.href, callback=self.detail_page2, fetch_type='js')
    
    @config(priority=3)
    def detail_page2(self, response):
        for each in response.doc('a[href^="http"]').items():
            self.crawl(each.attr.href, callback=self.detail_page3, fetch_type='js')
    
    @config(priority=4)
    def detail_page3(self, response):
        stringaa = response.text.encode("utf-8")  
        ret = stringaa.find('ActiveXObject') != -1
        if ret:
            return {
                "url": response.url,
                "text":response.doc.html(),
                "title": response.doc('title').text(),
           }



#!/usr/bin/env python  
# -*- encoding: utf-8 -*-  
# Created on 2017-07-01 16:47:26  
# Project: company_list  
  
from pyspider.libs.base_handler import *  
import re  
class Handler(BaseHandler):  
    crawl_config = {  
    }  
 
    @every(minutes=24 * 60)  
    def on_start(self):  
        self.crawl('https://companylist.org/categories/', callback=self.index_page,validate_cert = False)  
 
    @config(age=10 * 24 * 60 * 60)  
    def index_page(self, response):  
        for each in response.doc('a[href^="http"]').items():  
            self.crawl(each.attr.href, callback=self.detail_page,validate_cert=False)  
            
 
    @config(priority=2)  
    def detail_page(self, response):   
        nextUrl = response.doc('a[class="paginator-next"]').attr.href  
        if nextUrl:  
            self.crawl(nextUrl,callback=self.detail_page,validate_cert=False)  
        return {  
            "url": response.url,  
            "title": response.doc('title').text(),  
        } 



 #!/usr/bin/env python
# -*- encoding: utf-8 -*-
# Created on 2018-01-02 13:48:16
# Project: test

from pyspider.libs.base_handler import *

DIR_PATH = '/home/wdq/pyspider_data/result_school.txt'


class Handler(BaseHandler):
    crawl_config = {
    }

    @every(minutes=24 * 60)
    def on_start(self):
        self.crawl('http://172.30.28.133:8080/pyspider/school.html', callback=self.index_page)

    @config(age=-1)
    def index_page(self, response):
        self.detail_page3(response)
        for each in response.doc('a[href^="http"]').items():
            self.crawl(each.attr.href, callback=self.detail_page1, fetch_type='js')

    @config(priority=2)
    def detail_page1(self, response):
        self.detail_page3(response)
        for each in response.doc('a[href^="http"]').items():
            self.crawl(each.attr.href, callback=self.detail_page2, fetch_type='js')
    
    @config(priority=3)
    def detail_page2(self, response):
        self.detail_page3(response)
        for each in response.doc('a[href^="http"]').items():
            self.crawl(each.attr.href, callback=self.detail_page3, fetch_type='js')
    
    @config(priority=4)
    def detail_page3(self, response):
        stringaa = response.text.encode("utf-8")  
        #ret = stringaa.find('ActiveXObject') != -1
        ret = stringaa.find('classid="CLSID') != -1
        if ret:
            f = open(DIR_PATH, 'ab')
            f.write(response.text)
            f.close
