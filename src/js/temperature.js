// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

var apiKey = 'YOUR_API_KEY';

var xhrRequest = function(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {

    var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
    pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + apiKey;
    

    xhrRequest(url, 'GET',
               function(responseText) {

               var json = JSON.parse(responseText);
               
               // convert Kelvin
               var temperature = Math.round(json.main.temp - 273.15);
               
               var dictionary = {
               "KEY_TEMPERATURE": temperature
               };
               
               Pebble.sendAppMessage(dictionary,
                                     function(e) {
                                     console.log("Temperature successfully sent to Pebble");
                                     },
                                     function(e) {
                                     console.log("Error sending temperature to Pebble");
                                     }
                                     );
               }
               );
}

function locationError(err) {
    console.log("Error requesting location");
}

function getWeather() {
    navigator.geolocation.getCurrentPosition(
                                             locationSuccess,
                                             locationError, {
                                             timeout: 15000,
                                             maximumAge: 60000
                                             }
                                             );
}

Pebble.addEventListener('ready',
                        function(e) {
                        getWeather();
                        }
                        );

Pebble.addEventListener('appmessage',
                        function(e) {
                        getWeather();
                        }
                        );
