var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getWeatherSuccess(lat, lng, success, callback) {
  var url = 'http://pebble.heyo.me/weather?lat=' + lat + '&lng=' + lng;

  console.log('['+Date.now()+']'+'url:'+url);
  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);

      var dictionary = {
        'KEY_ACTION': 'weather',
        'KEY_WEATHER_TEMP': json.kion?''+json.kion:'0',
        'KEY_WEATHER_COND': (json.sky?json.sky:'') + (json.rain?' '+json.rain:''),
        'KEY_WEATHER_OVER': (''+json.rain).indexOf('비')!=-1?1:0,
        'KEY_WEATHER_POS': json.name?json.name:''
      };
      if (success) {
        dictionary.KEY_NAVI_LAT = (''+lat).substring(0, 17);
        dictionary.KEY_NAVI_LNG = (''+lng).substring(0, 17);
      }
      console.log('KEY_ACTION:'+dictionary.KEY_ACTION);
      //console.log('KEY_WEATHER_TEMP:'+dictionary.KEY_WEATHER_TEMP);
      //console.log('KEY_NAVI_LAT:'+dictionary.KEY_NAVI_LAT);
      //console.log('KEY_NAVI_LNG:'+dictionary.KEY_NAVI_LNG);

      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Successfully delivered message with transactionId=' + e.data.transactionId);          
        },
        function(e) {
          console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
        }
      );
    }      
  );
}

function getWeather(lat, lng, callback) {
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      getWeatherSuccess(pos.coords.latitude, pos.coords.longitude, true, callback);
    },
    function(err) {
      //locationError(err);
      if (lat && lng) {
        getWeatherSuccess(lat, lng, false, callback);
      } else {
        if(callback) callback();
      }
    },
    {timeout: 15000, maximumAge: 60000}
  );
}

function getCustomSuccess(lat, lng, success, callback) {
  var url = 'http://pebble.heyo.me/custom?lat=' + lat + '&lng=' + lng;

  // Send request
  console.log('['+Date.now()+']'+'url:'+url);
  xhrRequest(url, 'GET', 
    function(responseText) {
      //console.log('//getCustomSuccess');
      //console.log(responseText);
      var json = JSON.parse(responseText);

      var dictionary = {
        'KEY_ACTION': 'custom',
        'KEY_CUSTOM_HOME': json.home,
        'KEY_CUSTOM_MSG': json.msg
      };
      if (success) {
        dictionary.KEY_NAVI_LAT = (''+lat).substring(0, 17);
        dictionary.KEY_NAVI_LNG = (''+lng).substring(0, 17);
      }
      console.log('KEY_ACTION:'+dictionary.KEY_ACTION);
      console.log('KEY_NAVI_LAT:'+dictionary.KEY_NAVI_LAT);
      console.log('KEY_NAVI_LNG:'+dictionary.KEY_NAVI_LNG);
    
      console.log('$$');
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('['+Date.now()+']'+'Custom info sent to Pebble successfully!');
          if (callback) callback();
        },
        function(e) {
          console.log('['+Date.now()+']'+'Error sending custom info to Pebble!');
          if (callback) callback();
        }
      );
      console.log('##');

    }      
  );
}

function getCustom(lat, lng, callback) {
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      getCustomSuccess(pos.coords.latitude, pos.coords.longitude, true, callback);
    },
    function(err) {
      //locationError(err);
      if (lat && lng) {
        getCustomSuccess(lat, lng, false, callback);
      } else {
        if(callback) callback();
      }
    },
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    
    //getWeather(0, 0);
    Pebble.sendAppMessage({'KEY_ACTION':'ready'},
      function(e) {
        console.log('ready info sent to Pebble successfully!');
      },
      function(e) {
        console.log('Error sending ready info to Pebble!');
      }
    );
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('['+Date.now()+']'+' AppMessage received! ('+e.payload.KEY_ACTION+')');
    if (!e.payload) return null;
    console.log('KEY_NAVI_LAT:'+ e.payload.KEY_NAVI_LAT);
    console.log('KEY_NAVI_LNG:' + e.payload.KEY_NAVI_LNG);
    if (e.payload.KEY_ACTION=='weather') {
      getWeather(e.payload.KEY_NAVI_LAT, e.payload.KEY_NAVI_LNG);
    } else if (e.payload.KEY_ACTION=='custom') {
      getCustom(e.payload.KEY_NAVI_LAT, e.payload.KEY_NAVI_LNG);
    } else if (e.payload.KEY_ACTION=='all') {
      getWeather(e.payload.KEY_NAVI_LAT, e.payload.KEY_NAVI_LNG);
    } else if (e.payload.KEY_ACTION=='ready') {
      Pebble.sendAppMessage({'KEY_ACTION':'ready'},
        function(e) {
          console.log('ready info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending ready info to Pebble!');
        }
      );
    }
  }                     
);

