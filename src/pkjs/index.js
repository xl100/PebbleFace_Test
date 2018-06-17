var xhrRequest = function(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    // Construct URL
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=d219390cac522c99582f1e52712228f2';
    
    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET',
        function(responseText) {
            // ResponseText contains a JSON object with weather info
            var json = JSON.parse(responseText);
            
            // Temperature in Kelvin requires adjustment
            var temperature = Math.round((json.main.temp - 273.15) / 5 * 9 + 32);
            console.log('temperature is ' + temperature);
            
            // Conditions
            var conditions = json.weather[0].main;
            console.log('conditions are ' + conditions);
            
            // Assemble dictionary using our keys
            var dictionary = {
                'TEMPERATURE': temperature,
                'CONDITIONS': conditions
            };
            
            Pebble.sendAppMessage(dictionary,
                function(e) {
                    console.log('Weather info sent to Pebble successfully!');
                },
                function(e) {
                    console.log('Error sending weather info to Pebble!');
                }
            );
        }
    );
}

function locationError(err) {
    console.log('Error requesting location!');
}

function getWeather() {
    navigator.geolocation.getCurrentPosition(
        locationSuccess, locationError, {timeout: 15000, maximumAge: 60000}
    );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
    function(e) {
        console.log('PebbleKit JS ready!');
        
        // Get the inititial weather
        getWeather();
    }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
    function(e) {
        console.log('AppMessage received!');
        getWeather();
    }                     
);

Pebble.addEventListener('showConfiguration',
    function() {
        var url = 'localhost';
        
        console.log("Settings open");
        //Pebble.openURL(url);
    }
);