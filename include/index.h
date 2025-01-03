const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 Configuration</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background-color: #f4f4f9;
    }
    .container {
      width: 90%;
      max-width: 400px;
      padding: 20px;
      border-radius: 8px;
      background-color: #ffffff;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    h2 {
      text-align: center;
      color: #333;
    }
    label {
      font-size: 0.9rem;
      color: #555;
    }
    input[type="text"], input[type="password"], select {
      width: 100%;
      padding: 8px;
      margin: 8px 0;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    button {
      width: 100%;
      padding: 10px;
      border: none;
      border-radius: 4px;
      color: white;
      background-color: #4CAF50;
      cursor: pointer;
    }
    button:hover {
      background-color: #45a049;
    }
    .section {
      margin-top: 20px;
    }
    .status {
      text-align: center;
      margin-top: 20px;
      font-size: 0.9rem;
      color: #666;
    }
  </style>
</head>
<body>

<div class="container">
  <h2>ESP8266 Configuration</h2>

  <!-- MQTT and WiFi Configuration Form -->
  <form id="config-form" onsubmit="return saveConfig()">
    <label for="mqtt-host">MQTT Host</label>
    <input type="text" id="mqtt-host" name="mqttHost" required>

    <label for="mqtt-user">MQTT User</label>
    <input type="text" id="mqtt-user" name="mqttUser" required>

    <label for="mqtt-password">MQTT Password</label>
    <input type="password" id="mqtt-password" name="mqttPassword" required>

    <label for="mqtt-topic">MQTT Topic</label>
    <input type="text" id="mqtt-topic" name="mqttTopic" required>

    <label for="mqtt-fulltopic">MQTT Full Topic</label>
    <input type="text" id="mqtt-fulltopic" name="mqttFullTopic" required>

    <label for="ssid">SSID</label>
    <input type="text" id="ssid" name="ssid" required>

    <label for="wifi-password">WiFi Password</label>
    <input type="password" id="wifi-password" name="wifiPassword" required>

    <label for="protocol">Protocol</label>
    <select id="protocol" name="protocol" required>
      <option value="-1">UNKNOWN</option>
      <option value="0">UNUSED</option>
      <option value="1">RC5</option>
      <option value="2">RC6</option>
      <option value="3">NEC</option>
      <option value="4">SONY</option>
      <option value="5">PANASONIC</option>
      <option value="6">JVC</option>
      <option value="7">SAMSUNG</option>
      <option value="8">WHYNTER</option>
      <option value="9">AIWA_RC_T501</option>
      <option value="10">LG</option>
      <option value="11">SANYO</option>
      <option value="12">MITSUBISHI</option>
      <option value="13">DISH</option>
      <option value="14">SHARP</option>
      <option value="15">COOLIX</option>
      <option value="16">DAIKIN</option>
      <option value="17">DENON</option>
      <option value="18">KELVINATOR</option>
      <option value="19">SHERWOOD</option>
      <option value="20">MITSUBISHI_AC</option>
      <option value="21">RCMM</option>
      <option value="22">SANYO_LC7461</option>
      <option value="23">RC5X</option>
      <option value="24">GREE</option>
      <option value="25">PRONTO</option>
      <option value="26">NEC_LIKE</option>
      <option value="27">ARGO</option>
      <option value="28">TROTEC</option>
      <option value="29">NIKAI</option>
      <option value="30">RAW</option>
      <option value="31">GLOBALCACHE</option>
      <option value="32">TOSHIBA_AC</option>
      <option value="33">FUJITSU_AC</option>
      <option value="34">MIDEA</option>
      <option value="35">MAGIQUEST</option>
      <option value="36">LASERTAG</option>
      <option value="37">CARRIER_AC</option>
      <option value="38">HAIER_AC</option>
      <option value="39">MITSUBISHI2</option>
      <option value="40">HITACHI_AC</option>
      <option value="41">HITACHI_AC1</option>
      <option value="42">HITACHI_AC2</option>
      <option value="43">GICABLE</option>
      <option value="44">HAIER_AC_YRW02</option>
      <option value="45">WHIRLPOOL_AC</option>
      <option value="46">SAMSUNG_AC</option>
      <option value="47">LUTRON</option>
      <option value="48">ELECTRA_AC</option>
      <option value="49">PANASONIC_AC</option>
      <option value="50">PIONEER</option>
      <option value="51">LG2</option>
      <option value="52">MWM</option>
      <option value="53">DAIKIN2</option>
      <option value="54">VESTEL_AC</option>
      <option value="55">TECO</option>
      <option value="56">SAMSUNG36</option>
      <option value="57">TCL112AC</option>
      <option value="58">LEGOPF</option>
      <option value="59">MITSUBISHI_HEAVY_88</option>
      <option value="60">MITSUBISHI_HEAVY_152</option>
      <option value="61">DAIKIN216</option>
      <option value="62">SHARP_AC</option>
      <option value="63">GOODWEATHER</option>
      <option value="64">INAX</option>
      <option value="65">DAIKIN160</option>
      <option value="66">NEOCLIMA</option>
      <option value="67">DAIKIN176</option>
      <option value="68">DAIKIN128</option>
      <option value="69">AMCOR</option>
      <option value="70">DAIKIN152</option>
      <option value="71">MITSUBISHI136</option>
      <option value="72">MITSUBISHI112</option>
      <option value="73">HITACHI_AC424</option>
      <option value="74">SONY_38K</option>
      <option value="75">EPSON</option>
      <option value="76">SYMPHONY</option>
      <option value="77">HITACHI_AC3</option>
      <option value="78">DAIKIN64</option>
      <option value="79">AIRWELL</option>
      <option value="80">DELONGHI_AC</option>
      <option value="81">DOSHISHA</option>
      <option value="82">MULTIBRACKETS</option>
      <option value="83">CARRIER_AC40</option>
      <option value="84">CARRIER_AC64</option>
      <option value="85">HITACHI_AC344</option>
      <option value="86">CORONA_AC</option>
      <option value="87">MIDEA24</option>
      <option value="88">ZEPEAL</option>
      <option value="89">SANYO_AC</option>
      <option value="90">VOLTAS</option>
      <option value="91">METZ</option>
      <option value="92">TRANSCOLD</option>
      <option value="93">TECHNIBEL_AC</option>
      <option value="94">MIRAGE</option>
      <option value="95">ELITESCREENS</option>
      <option value="96">PANASONIC_AC32</option>
      <option value="97">MILESTAG2</option>
      <option value="98">ECOCLIM</option>
      <option value="99">XMP</option>
      <option value="100">TRUMA</option>
      <option value="101">HAIER_AC176</option>
      <option value="102">TEKNOPOINT</option>
      <option value="103">KELON</option>
      <option value="104">TROTEC_3550</option>
      <option value="105">SANYO_AC88</option>
      <option value="106">BOSE</option>
      <option value="107">ARRIS</option>
      <option value="108">RHOSS</option>
      <option value="109">AIRTON</option>
      <option value="110">COOLIX48</option>
      <option value="111">HITACHI_AC264</option>
      <option value="112">KELON168</option>
      <option value="113">HITACHI_AC296</option>
      <option value="114">DAIKIN200</option>
      <option value="115">HAIER_AC160</option>
      <option value="116">CARRIER_AC128</option>
      <option value="117">TOTO</option>
      <option value="118">CLIMABUTLER</option>
      <option value="119">TCL96AC</option>
      <option value="120">BOSCH144</option>
      <option value="121">SANYO_AC152</option>
      <option value="122">DAIKIN312</option>
      <option value="123">GORENJE</option>
      <option value="124">WOWWEE</option>
      <option value="125">CARRIER_AC84</option>
      <option value="126">YORK</option>
    </select>

    <button type="submit">Save Configuration</button>
  </form>

  <!-- IR Protocol Display and Test Command -->
  <div class="section">
    <h3>IR Control</h3>
    <div class="status" id="ir-detected">Detected Protocol: <span id="detected-protocol">None</span></div>
 <!-- Protocol Selection Dropdown -->
 <label for="test-protocol">Select Protocol:</label>
 <select id="test-protocol" name="protocol" required>
   <option value="-1">UNKNOWN</option>
   <option value="0">UNUSED</option>
   <option value="1">RC5</option>
   <option value="2">RC6</option>
   <option value="3">NEC</option>
   <option value="4">SONY</option>
   <option value="5">PANASONIC</option>
   <option value="6">JVC</option>
   <option value="7">SAMSUNG</option>
   <option value="8">WHYNTER</option>
   <option value="9">AIWA_RC_T501</option>
   <option value="10">LG</option>
   <option value="11">SANYO</option>
   <option value="12">MITSUBISHI</option>
   <option value="13">DISH</option>
   <option value="14">SHARP</option>
   <option value="15">COOLIX</option>
   <option value="16">DAIKIN</option>
   <option value="17">DENON</option>
   <option value="18">KELVINATOR</option>
   <option value="19">SHERWOOD</option>
   <option value="20">MITSUBISHI_AC</option>
   <option value="21">RCMM</option>
   <option value="22">SANYO_LC7461</option>
   <option value="23">RC5X</option>
   <option value="24">GREE</option>
   <option value="25">PRONTO</option>
   <option value="26">NEC_LIKE</option>
   <option value="27">ARGO</option>
   <option value="28">TROTEC</option>
   <option value="29">NIKAI</option>
   <option value="30">RAW</option>
   <option value="31">GLOBALCACHE</option>
   <option value="32">TOSHIBA_AC</option>
   <option value="33">FUJITSU_AC</option>
   <option value="34">MIDEA</option>
   <option value="35">MAGIQUEST</option>
   <option value="36">LASERTAG</option>
   <option value="37">CARRIER_AC</option>
   <option value="38">HAIER_AC</option>
   <option value="39">MITSUBISHI2</option>
   <option value="40">HITACHI_AC</option>
   <option value="41">HITACHI_AC1</option>
   <option value="42">HITACHI_AC2</option>
   <option value="43">GICABLE</option>
   <option value="44">HAIER_AC_YRW02</option>
   <option value="45">WHIRLPOOL_AC</option>
   <option value="46">SAMSUNG_AC</option>
   <option value="47">LUTRON</option>
   <option value="48">ELECTRA_AC</option>
   <option value="49">PANASONIC_AC</option>
   <option value="50">PIONEER</option>
   <option value="51">LG2</option>
   <option value="52">MWM</option>
   <option value="53">DAIKIN2</option>
   <option value="54">VESTEL_AC</option>
   <option value="55">TECO</option>
   <option value="56">SAMSUNG36</option>
   <option value="57">TCL112AC</option>
   <option value="58">LEGOPF</option>
   <option value="59">MITSUBISHI_HEAVY_88</option>
   <option value="60">MITSUBISHI_HEAVY_152</option>
   <option value="61">DAIKIN216</option>
   <option value="62">SHARP_AC</option>
   <option value="63">GOODWEATHER</option>
   <option value="64">INAX</option>
   <option value="65">DAIKIN160</option>
   <option value="66">NEOCLIMA</option>
   <option value="67">DAIKIN176</option>
   <option value="68">DAIKIN128</option>
   <option value="69">AMCOR</option>
   <option value="70">DAIKIN152</option>
   <option value="71">MITSUBISHI136</option>
   <option value="72">MITSUBISHI112</option>
   <option value="73">HITACHI_AC424</option>
   <option value="74">SONY_38K</option>
   <option value="75">EPSON</option>
   <option value="76">SYMPHONY</option>
   <option value="77">HITACHI_AC3</option>
   <option value="78">DAIKIN64</option>
   <option value="79">AIRWELL</option>
   <option value="80">DELONGHI_AC</option>
   <option value="81">DOSHISHA</option>
   <option value="82">MULTIBRACKETS</option>
   <option value="83">CARRIER_AC40</option>
   <option value="84">CARRIER_AC64</option>
   <option value="85">HITACHI_AC344</option>
   <option value="86">CORONA_AC</option>
   <option value="87">MIDEA24</option>
   <option value="88">ZEPEAL</option>
   <option value="89">SANYO_AC</option>
   <option value="90">VOLTAS</option>
   <option value="91">METZ</option>
   <option value="92">TRANSCOLD</option>
   <option value="93">TECHNIBEL_AC</option>
   <option value="94">MIRAGE</option>
   <option value="95">ELITESCREENS</option>
   <option value="96">PANASONIC_AC32</option>
   <option value="97">MILESTAG2</option>
   <option value="98">ECOCLIM</option>
   <option value="99">XMP</option>
   <option value="100">TRUMA</option>
   <option value="101">HAIER_AC176</option>
   <option value="102">TEKNOPOINT</option>
   <option value="103">KELON</option>
   <option value="104">TROTEC_3550</option>
   <option value="105">SANYO_AC88</option>
   <option value="106">BOSE</option>
   <option value="107">ARRIS</option>
   <option value="108">RHOSS</option>
   <option value="109">AIRTON</option>
   <option value="110">COOLIX48</option>
   <option value="111">HITACHI_AC264</option>
   <option value="112">KELON168</option>
   <option value="113">HITACHI_AC296</option>
   <option value="114">DAIKIN200</option>
   <option value="115">HAIER_AC160</option>
   <option value="116">CARRIER_AC128</option>
   <option value="117">TOTO</option>
   <option value="118">CLIMABUTLER</option>
   <option value="119">TCL96AC</option>
   <option value="120">BOSCH144</option>
   <option value="121">SANYO_AC152</option>
   <option value="122">DAIKIN312</option>
   <option value="123">GORENJE</option>
   <option value="124">WOWWEE</option>
   <option value="125">CARRIER_AC84</option>
   <option value="126">YORK</option>
 </select>
    <button onclick="sendTestCommand()">Send Test IR Command</button>
  </div>

  <div class="status" id="status-message"></div>
</div>

<script>
  function saveConfig() {
    const mqttHost = document.getElementById('mqtt-host').value;
    const mqttUser = document.getElementById('mqtt-user').value;
    const mqttPassword = document.getElementById('mqtt-password').value;
    const mqttTopic = document.getElementById('mqtt-topic').value;
    const mqttFullTopic = document.getElementById('mqtt-fulltopic').value;
    const ssid = document.getElementById('ssid').value;
    const wifiPassword = document.getElementById('wifi-password').value;
    const protocol = document.getElementById('protocol').value;
    const url = `http://192.168.4.1/cm?user=admin&cmnd=${encodeURIComponent(`Backlog MqttHost ${mqttHost}; MqttUser ${mqttUser}; MqttClient ${mqttUser}; MqttPassword ${mqttPassword}; Topic ${mqttTopic}; FullTopic ${mqttFullTopic}; SSID1 ${ssid}; Password1 ${wifiPassword}; Protocol ${protocol};`)}`
    fetch(url)
      .then(response => {
        if (response.ok) {
          document.getElementById('status-message').innerText = 'Configuration Saved!';
        } else {
          document.getElementById('status-message').innerText = 'Failed to Save Configuration.';
        }
      })
      .catch(error => {
        document.getElementById('status-message').innerText = 'Error saving configuration.';
        console.error('Error:', error);
      });
    return false;
  }

  function sendTestCommand() {
    const protocol = document.getElementById('test-protocol').value;
    const url = `http://192.168.4.1/testIR?command=protocol%20${protocol}%3B%20power%201%3B%20temp%2021%3B%20fan_speed%202%3B`;
    fetch(url)
      .then(response => {
        if (response.ok) {
          document.getElementById('status-message').innerText = 'Test IR Command Sent!';
        } else {
          document.getElementById('status-message').innerText = 'Failed to Send Test IR Command.';
        }
      })
      .catch(error => {
        document.getElementById('status-message').innerText = 'Error sending test IR command.';
        console.error('Error:', error);
      });
  }

  function fetchStatus() {
    fetch('http://192.168.4.1/status')
      .then(response => response.json())
      .then(data => {
        document.getElementById('detected-protocol').innerText = data.detectedProtocol || 'None';
      })
      .catch(error => {
        console.error('Error fetching status:', error);
      });
  }
  // Periodically call fetchStatus to update detected and selected protocols
  setInterval(fetchStatus, 5000); // Fetch every 5 seconds
</script>
</body>
</html>
)rawliteral";

String homePageSimple = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>Home page</head>
<body>simple home page for testing</body>
</html>
)=====";