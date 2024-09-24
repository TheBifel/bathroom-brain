// webpage.h
const char webpage[] = R"rawliteral(
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<style>
    body {
        font-family: Arial, sans-serif;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
        background-color: #f4f4f4;
        overflow-x: hidden;
    }
    .container {
        text-align: center;
        background: white;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        width: 100%;
        max-width: 500px;
        box-sizing: border-box;
    }
    .row {
        display: flex;
        justify-content: center;
        align-items: center;
        margin-bottom: 10px;
    }
    .card {
        background-color: #f9f9f9;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        margin-bottom: 20px;
        width: 100%;
        box-sizing: border-box;
    }
    button {
        margin: 10px;
        padding: 15px 20px;
        font-size: 1rem;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: background-color 0.3s;
    }
    .on {
        background-color: #28a745;
        color: white;
    }
    .off {
        background-color: blue;
        color: white;
    }
    input {
        padding: 15px;
        margin: 10px;
        border: 1px solid #ccc;
        border-radius: 5px;
        font-size: 1rem;
        width: calc(100% - 40px);
    }
    /* Media query for smaller devices */
    @media screen and (max-width: 600px) {
        body {
            height: auto;
            justify-content: flex-start;
            padding: 10px;
        }
        .container {
            width: 100vw;
            height: auto;
            max-width: none;
            border-radius: 0;
            padding: 10px;
        }
        button {
            width: 100%;
            font-size: 1.2rem;
        }
        input {
            font-size: 1.2rem;
            width: calc(100% - 20px);
        }
    }
</style>
</head>
<body>
<div class='container'>
<p>Went sensor reading: <span id='wentSensorReading'>null</span></p>
<p>Light sensor reading: <span id='lightSensorReading'>null</span></p>
<p>Toggle went in: <span id='delayToToggle'>N/A</span></p>

<div class='row'>
<button onclick='changeState()'>Toggle went</button>
</div>

<div class='row'>
<button id='lampButton' class='off' onclick='toggleMirrorLight()'>Mirror Light: Off</button>
<button id='heaterButton' class='off' onclick='toggleMirrorHeater()'>Mirror Heater: Off</button>
</div>

<div class='card'>
<div class='row'>
<label for='delayByLightsInput'>Enter delay after turning lights off (1-60 minutes):</label>
</div>
<div class='row'>
<input type='text' id='delayByLightsInput' pattern='[0-9]*' placeholder='1-60' value='null'>
<button onclick='sendDelayByLights()'>Accept</button>
</div>
</div>

<div class='card'>
<div class='row'>
<label for='delayInput'>Enter delay (1-60 minutes):</label>
</div>
<div class='row'>
<input type='text' id='delayInput' pattern='[0-9]*' placeholder='1-60' value='null'>
<button onclick='sendDelay()'>Accept</button>
</div>
</div>

</div>
<script>
// Add your JavaScript functions here (e.g., updatePageWithData, etc.)
</script>
</body>
</html>
)rawliteral";
