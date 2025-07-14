#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "GlobeAtHome_17400";
const char* password = "3ML454H26Q5";

// WebSocket Server on port 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Web server on port 80 for the web page
AsyncWebServer server(80);

// Parking data (6 slots)
String parkingSlots[6] = {"Available", "Available", "Available", "Available", "Available", "Available"};

// Sensor pin setup for Slot1 and Slot2 (Floor 1)
#define TRIG1 26
#define ECHO1 27
#define TRIG2 32
#define ECHO2 33

// LED pin setup for Sensor 1
#define RED_LED1 5
#define GREEN_LED1 18

// LED pin setup for Sensor 2
#define RED_LED2 19
#define GREEN_LED2 21


// HTML page with IDs F1A1, F1A2, etc.
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Parking System</title>
  <style>
    html, body {
      margin: 0;
      padding: 0;
      height: 100%;
      font-family: Arial, sans-serif;
      background-color: rgb(24, 25, 36);
      color: white;
      overflow: hidden;
    }

    .tabs {
      display: flex;
      justify-content: space-evenly;
      align-items: center;
      height: 60px;
      margin: 0;
      width: 100%;
      position: fixed;
      top: 0;
      z-index: 10;
    }

    .main-tab {
      padding: 10px 10px;
      background-color: #02041f;
      color: white;
      cursor: pointer;
      border-radius: 5px;
      text-align: center;
      border: 2px solid transparent;
      box-sizing: border-box;
    }

    .main-tab.active {
      border: 2px solid #00ffae ;
      background-color: transparent;
    }

/* Hide by default (phones and tablets) */
    .summary-tab {
      position: fixed;
      top: 30%;
      right: 0;
      transform: translateY(-50%);
      font-size: 30px;
      cursor: pointer;
      z-index: 1000;
      display: none; /* Hidden by default */
      justify-content: center;
      align-items: center;
      width: min-content;
      pointer-events: auto;
      border-radius: 5px;
      padding: 5px 10px;
      height: min-content;
      background: none; 
      color: #333;
    }

/* Show only on screens wider than 1024px */
    @media (min-width: 1024px) {
      .summary-tab {
        display: flex;
      }
    }

/* First floor */
    .map-container {
      position: absolute;
      top: 60px; 
      bottom: 0;
      left: 0;
      right: 0;
      overflow: hidden;
      touch-action: none;
    }

    .floor-map {
      display: none;
      width: 1200px;
      height: 800px;
      position: absolute;
      top: 100px;
      left: -223.667px;
      cursor: grab;
    }

    .floor-map.active {
      display: block;
    }

    .lot {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    justify-content: center;
    gap: 3px;
    width: fit-content;
    border-bottom: 1px solid whitesmoke;
    }  

    .column {
    display: flex;
    flex-direction: column-reverse; /* Stack from bottom */
    }

    .main-portion , .additional-portion {
    display: flex;
    flex-direction: row;
    }

    .additional-portion {
    display: flex;
    flex-direction: row;
    align-self: self-end;
    height: 25px;
    }

    .spot {
        width: 75px;
        height: 48px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 0.75rem;
        background-color: #069165;
    }

    .spot.occupied {
        background-color: #b30909;
    }

  .pathway {
      width: 100px;
      margin: 0 10px;
      display: flex;
      justify-content: center;
      align-items: center;
      position: relative;
  }

  .pathway {
      width: 100px;
      margin: 0 10px;
      display: flex;
      justify-content: center;
      align-items: center;
      position: relative;
      z-index: 1; 
  }

  .pathway::before {
      content: "";
      position: absolute;
      top: 0;
      height: 100%;
      left: 50%;
      transform: translateX(-50%);
      width: 2px;
      background-image: repeating-linear-gradient(
          yellow 0 10px,
          transparent 10px 20px
      );
      animation: pulseDash 1.5s ease-in-out infinite;
      z-index: 2;
      pointer-events: none;
  }

/* Reuse pulse animation */
    @keyframes pulseDash {
        0% {
            opacity: 1;
        }
        50% {
            opacity: 0.4;
        }
        100% {
            opacity: 1;
        }
    }

    .path {
        width: 100px;
        margin: 0 10px;
        display: flex;
        justify-content: center;
        align-items: center;
        position: relative; 
        z-index: 1; 
    }

    .additional-row {
        margin-top: 20px; 
        display: flex;
        justify-content: center; 
        flex-direction: column;
    }

    .additional-row .column {
        flex-direction: column-reverse; 
        gap: 3px;
    }

    * {
        box-sizing: border-box;
    }

/* Hazard striping for Zone 1 and Zone 2 */
    .zone {
        background: repeating-linear-gradient(
            45deg,
            black,
            black 10px,
            yellow 10px,
            yellow 20px
        );
        color: black; 
        font-weight: bold;
        padding: 10px; 
    }

    .zone-1 {
        border-bottom-left-radius: 35px;
        border-bottom-right-radius: 35px;
        width: 153px;
    }

    .zone-2 {
        border-bottom-left-radius: 35px;
        width: 78px;
    }

    .main-path {
        position: relative;
        height: 100px;
        width: 100%;
        display: flex;
        justify-content: center;
        align-items: center;
    }

    .main-pathway::before {
        content: "";
        position: absolute;
        top: 50%;
        left: 0;
        right: 0;
        height: 2px;
        width: 688px;
        background-image: repeating-linear-gradient(
            to right,
            yellow 0 10px,
            transparent 10px 20px
        );
        transform: translateY(-50%);
        animation: pulseDash 1.5s ease-in-out infinite;
    }

    @keyframes pulseDash {
        0% {
            opacity: 1;
        }
        50% {
            opacity: 0.4;
        }
        100% {
            opacity: 1;
        }
    }

    .divider {
      height: 3px;
      background-color: white;
    }

    .spot.selected {
        background-color: transparent; 
        border: 2px solid #2563eb; 
        box-shadow: inset 0 0 8px #2563eb, inset 0 0 16px #2563eb; 
        color: #ffffff;
        transition: 0.3s;
    }

    .bottom-buttons-container {
      position: fixed;
      bottom: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-between;
      z-index: 10;
      width: 100%;
      background-color: transparent;
      pointer-events: none;
      height: auto; 
    }

    .indicator-container {
      display: flex;
      gap: 30px;
      justify-content: center;
      align-items: center;
      margin-bottom: 10px;
    }

    .indicator-label {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: .80rem;
    }

/* Left and Right Features Styling */
    .left-right-container {
      display: flex;
      justify-content: space-between; 
      width: 100%;
      padding: 0px 12px;
      background-color: #02041f;
      border-radius: 10px;
      height: 182px;
      gap: 12px;
      background-color: transparent;
      flex-wrap: nowrap; /* Prevent wrapping */
    }

    .left-feature, .right-feature {
      display: flex;
      flex-direction: column;
      align-items: center;
      background-color: #02041f;
      padding: 12px 15px 0px;
      border-radius: 10px;
      flex: 1; 
    }

    .left-feature {
      background-color: #02041f;
    }

    .floor-label {
      font-size: .9rem;
      font-weight: 500;
      margin-top: 22px;
    }

    .right-feature {
      background-color: #02041f; 
    }

    .label {
      font-size: .85rem;
      margin-bottom: 25px; 
      color: #f0f0f0;
      text-align: left;
      width: 100%; 
    }

    .circle-wrapper {
      position: relative;
      width: 80px;
      height: 80px;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .circle-container {
      width: 100%;
      height: 100%;
      border-radius: 50%;
      background: conic-gradient(
        #00ffae 0% 69.9%,  /* Green from 0% to just before 70% */
        #FF0033 70% 100%  /* Red starts cleanly at 70% and finishes at 100% */
      ); 
      animation: pulse 1.5s infinite alternate, glow 2s infinite alternate; 
      position: absolute;
      top: 0;
      left: 0;
      box-shadow: 0 0 15px rgba(0, 255, 174, 1); /* Glowing effect */
    }

/* Glowing animation to make the glow effect pulse */
    @keyframes glow {
      0% {
        box-shadow: 0 0 15px rgba(0, 255, 174, 1);
      }
      50% {
        box-shadow: 0 0 30px rgba(0, 255, 174, 1);
      }
      100% {
        box-shadow: 0 0 15px rgba(0, 255, 174, 1);
      }
    }

/* Animation for the inner circle pulse */
    @keyframes pulse {
      0% {
        transform: scale(1);
      }
      100% {
        transform: scale(1.05);
      }
    }

    .circle-container::before {
      content: '';
      position: absolute;
      width: 65px;
      height: 65px;
      background-color: #02041f;
      border-radius: 50%;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
    }

    .circle {
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      color: white;
      font-size: 1.2rem;
      font-weight: 500;
      z-index: 1;
    }

    .spot-labels {
      display: grid;
      grid-template-columns: repeat(2, 1fr); /* Two columns */
      gap: 10px;
      grid-template-rows: 1fr 1fr; /* Always 2 rows, even when no data */
      position: relative;
      min-height: 4.2rem;
    }

    .spot-labels > div {
      /* Ensures that only actual items take space */
      min-height: 0;
    }

    .spot-labels span {
      display: block; /* make each span a block-level element */
      text-align: center; /* center the text */
      padding: 5px;
      border-radius: 5px;
    }

    .new-container {
      width: calc(100% - 24px); /* 12px left + 12px right */
      margin: 12px;
      display: flex;
      justify-content: center;
      background-color: #02041f;
      border-radius: 10px;
    }

    .new-feature {
      padding: 15px 0px;
      width: 80%;
      text-align: center;
      height: 182px;
    }

    .feature-content {
      font-size: 1rem;
      color: #f0f0f0;
      margin-top: 12px;
      height: 75%;
    }

    .expanded {
      flex: 2; /* This will make Zone 2 twice as wide */
    }

/* Parking Summary Page Styles */
    .summary-container {
      display: none;
      padding: 20px;
    }

    .summary-container.active {
        display: block;
    }

    .summary-container h1 {
      font-size: 2em;
      margin-bottom: 10px;
    }

    .summary-container p {
      font-size: 1.2em;
      line-height: 1.5;
    }

/* Second floor */
/* Apply border-bottom-left-radius only to Zone 2 */
    .portion-F2 {
      display: flex;
      flex-direction: row;
      display: flex;
      flex-direction: row;
      align-self: self-start;
      height: 25px;
    }

    .zoneF2-1 {
      border-bottom-right-radius: 35px;
      border-top-right-radius: 5px;
      width: 78px;
    }

    .zoneF2-2 {
      border-bottom-left-radius: 35px;
      border-bottom-right-radius: 35px;
      border-top-left-radius: 5px;
      border-top-right-radius: 5px;
      width: 153px;
    }

    .zoneF2-3 {
      border-bottom-left-radius: 35px;
      border-bottom-right-radius: 35px;
      border-top-left-radius: 5px;
      border-top-right-radius: 5px;
      width: 153px;
    }

    .zoneF2-1 {
      border-bottom-right-radius: 35px;
      width: 78px;
    }

    .zoneF2-2 {
      border-bottom-left-radius: 35px;
      border-bottom-right-radius: 35px;
      width: 153px;
    }

    .zoneF2-3 {
      border-bottom-left-radius: 35px;
      border-bottom-right-radius: 35px;
      width: 153px;
    }

    .main-path {
      position: relative;
      height: 100px;
      width: 100%;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .pathway-F2::before {
        content: "";
        position: absolute;
        top: 50%;
        left: 0;
        right: 0;
        height: 2px;
        width: 688px;
        background-image: repeating-linear-gradient(
          to right,
          yellow 0 10px,
          transparent 10px 20px
        );
        transform: translateY(-50%);
        animation: pulseDash 1.5s ease-in-out infinite;
    }

    @keyframes pulseDash {
      0% {
        opacity: 1;
      }
      50% {
        opacity: 0.4;
      }
      100% {
        opacity: 1;
      }
    }

    .floor-map {
      display: none;
    }

    .floor-map.active {
      display: block;
    }

    .popup {
      position: absolute; 
      top: -100%;
      left: 0;
      width: calc(100% - 24px); 
      background-color: #ff4444;
      color: white;
      padding: 15px 0;
      margin-left: 12px;
      margin-right: 12px;
      text-align: center;
      font-weight: bold;
      opacity: 0;
      transition: opacity 0.4s ease;
      z-index: 1000;
      border-radius: 10px;
    }

    .popup.show {
      opacity: 1; 
      top: 0;
    }

    .bottom-buttons-container.show-popup {
      padding-top: 80px; 
    }

    .hidden {
      display: none;
    }

/* Loading Indicator Fullscreen Overlay */
    .loading-overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: #02041f;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      font-size: 1.2em;
      z-index: 9999;
      opacity: 1;
      transition: opacity 0.5s ease;
      pointer-events: none;
    }

/* Fade out when data is loaded */
    .loading-overlay.fade-out {
      opacity: 0;
      transition: opacity 0.5s ease;
    }

/* Spinner */
    .loading-spinner {
      border: 6px solid #f3f3f3;
      border-top: 6px solid #2563eb; /* Changed spinning color to #2563eb */
      border-radius: 50%;
      width: 50px;
      height: 50px;
      animation: spin 1s linear infinite;
      margin-bottom: 12px;
    }

/* Spin animation */
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }

    .main-container,
    .summary-container,
    .bottom-buttons-container {
      display: none;
    }
    .main-container.active,
    .summary-container.active,
    .bottom-buttons-container.active {
      display: block;
    }

/* ------------------------- SUMMARY PAGE ------------------------- */
    .summary-page {
      background-color: rgb(24, 25, 36);
      height: 100vh;
      width: 100vw;
      position: fixed;
      top: 0;
      left: 0;
      display: flex;
      padding-top: 60px;
      box-sizing: border-box;
      overflow: hidden;
      flex-direction: row;
    }

/* Top bar styles */
    .top-bar {
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      height: 50px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      background-color: #02041f;
      z-index: 1000;
      padding: 10px 15px;
      box-sizing: border-box;
      color: white;
    }

    .left-group {
      display: flex;
      align-items: center;
    }

    .top-button.back-button,
    .top-button.fullscreen-button,
    .top-button.dropdown {
      background: none;
      border: none;
      color: white;
      font-size: clamp(0.9rem, 1.8vw, 1.2rem);
      cursor: pointer;
      padding: 10px;
    }

    .top-button.fullscreen-button {
      margin-left: 10px;
    }

    .dropdown {
      margin-right: 0;
      color: white;
    }

    .dropdown option {
      background-color: #02041f;
      color: white;
      border: none;
    }

    .dropdown option:hover {
      background-color: #d0d0d0;
    }

    .parking-title {
      color: white;
      font-size: clamp(1.5rem, 5vw, 3rem);
      font-weight: bold;
      margin-bottom: 10px;
      text-align: center;
    }

/* Remove focus/active styling for buttons and select dropdowns */
    .top-button:focus,
    .top-button:active,
    .dropdown:focus,
    .dropdown:active {
      outline: none;
      box-shadow: none;
    }

/* ------------------------- CONTENT COLUMNS ------------------------- */
    .summary-left {
      width: 75%;
      padding: 0 15px;
      height: calc(100vh - 60px);
      overflow: hidden;
      box-sizing: border-box;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }

    .summary-right {
      width: 25%;
      background-color: rgb(24, 25, 36);
      padding: 15px;
      box-sizing: border-box;
      height: calc(100vh - 60px);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      gap: 60px;
    }

    body.fullscreen-mode .summary-left,
    body.fullscreen-mode .summary-right {
      height: 100vh;
      justify-content: center;
    }

/* ------------------------- SPOT LIST ------------------------- */
    .summary-spots {
      flex-grow: 1;
      height: 100%;
      overflow: hidden;
    }

    .parking-list {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); /* reduced from 200px */
      gap: 20px; /* smaller gap */
      font-weight: bold;
      margin-top: 30px;
      margin-bottom: 30px;
      width: 180px
    }

    .parking-spot {
      padding: 15px;
      text-align: center;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 90px;
      font-size: clamp(1rem, 4vw, 3rem);
      color: black;
      background-color: #e0e0e0;
      border-radius: 10px;
    }

/* ------------------------- FLOOR SUMMARY CARDS ------------------------- */
    .floor-summary-card {
      background-color: #02041f;
      color: white;
      padding: 12px;
      border-radius: 10px;
      font-size: clamp(1.2rem, 2.5vw, 2rem);
      box-shadow: 0 0 8px rgba(0, 0, 0, 0.1);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      width: 100%;
      height: 190px;
      position: relative;
    }

    .floor-summary-card h3 {
      position: absolute;
      top: 15px;
      left: 20px;
      margin: 0;
      font-size: clamp(1rem, 1.5vw, 1.4rem);
      text-align: left;
      color: white;
    }

    .floor-summary-card .slot-count {
      font-size: clamp(2.5rem, 8vw, 5rem);
      font-weight: bold;
      text-align: center;
      margin-top: auto;
      margin-bottom: auto;
      color: #00ffae;
    }

/* ------------------------- RESPONSIVE ------------------------- */
    @media (max-width: 768px) {
      .summary-page {
        flex-direction: column;
        padding-top: 60px;
      }

      .summary-left {
        width: 100%;
        height: 65vh;
        padding: 15px;
        box-sizing: border-box;
      }

      .summary-right {
        width: calc(100% - 24px);
        margin: 0 12px 12px 12px;
        height: 45vh;
        padding: 0;
        box-sizing: border-box;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 12px;
      }

      .parking-list {
        grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
      }

      .parking-spot {
        width: 140px;
        height: 70px;
        font-size: clamp(1rem, 4vw, 2rem);
        margin: 0;
      }

      .parking-title {
        margin-top: 20px;
        font-size: clamp(1.5rem, 5vw, 2.5rem);
      }

      .floor-summary-card {
        height: auto;
        width: 100%;
        flex: 1;
      }

          .fullscreen-button {
          display: none;
    }
    }

    @media (max-width: 480px) {
      .parking-list {
        grid-template-columns: 1fr;
      }

      .parking-spot {
        width: 100px;
        height: 50px;
        font-size: clamp(1rem, 4vw, 2rem);
        margin: 0;
      }

      .parking-title {
        margin-top: 15px;
      }

      .floor-summary-card {
        height: 130px;
      }
    }

</style>
</head>
<body>

<script>

function hideLoadingIndicator() {
  const loader = document.getElementById('loading-indicator');
  if (!loader) return;

  loader.classList.add('fade-out');
  setTimeout(() => {
    loader.style.display = 'none';
  }, 500);
}

// Function to go to the summary page
function goToSummary() {
  console.log('Go to summary function triggered');
  
  // Show the summary section
  document.querySelector('.summary-container')?.classList.add('active');
  
  // Hide the main content section
  document.querySelector('.main-container')?.classList.remove('active');
  document.querySelector('.bottom-buttons-container')?.classList.remove('active');

  // Automatically display first floor data
  updateParkingSummary(floorNumber);
}

// Function to go back to the main page from the summary
function goBackToMain() {
  console.log('Go back to main function triggered');
  
  // Show the main content section
  document.querySelector('.main-container')?.classList.add('active');
  
  // Hide the summary section
  document.querySelector('.summary-container')?.classList.remove('active');
  document.querySelector('.bottom-buttons-container')?.classList.add('active');
}

let originalTopBarStyle = '';  // Variable to store the original top bar style

// Full Screen
function toggleFullScreen() {
  const doc = document.documentElement;
  const topBar = document.querySelector('.top-bar');  // Select the top bar

  if (!document.fullscreenElement) {

    // Enter fullscreen
    if (doc.requestFullscreen) doc.requestFullscreen();
    else if (doc.webkitRequestFullscreen) doc.webkitRequestFullscreen();
    else if (doc.msRequestFullscreen) doc.msRequestFullscreen();
    
    // Store the original style of the top bar before fullscreen
    if (topBar) {
      originalTopBarStyle = topBar.getAttribute('style') || '';  // Store the current inline style
      topBar.style.display = 'none';  // Hide the top bar in fullscreen mode
    }

    document.body.classList.add('fullscreen');

  } else {
    // Exit fullscreen
    if (document.exitFullscreen) document.exitFullscreen();
    else if (document.webkitExitFullscreen) document.webkitExitFullscreen();
    else if (document.msExitFullscreen) document.msExitFullscreen();

    // Restore the original style of the top bar after exiting fullscreen
    if (topBar) {
      topBar.setAttribute('style', originalTopBarStyle);  // Restore the original inline style
    }
    document.body.classList.remove('fullscreen');
  }
}

// Listen for fullscreen change events to ensure top bar is restored correctly
document.addEventListener('fullscreenchange', function () {
  const topBar = document.querySelector('.top-bar');
  if (!document.fullscreenElement && topBar) {
    topBar.setAttribute('style', originalTopBarStyle);  // Restore the original style when exiting fullscreen
  }
});

document.addEventListener('webkitfullscreenchange', function () {
  const topBar = document.querySelector('.top-bar');
  if (!document.webkitFullscreenElement && topBar) {
    topBar.setAttribute('style', originalTopBarStyle);  // Restore the original style when exiting fullscreen
  }
});

document.addEventListener('msfullscreenchange', function () {
  const topBar = document.querySelector('.top-bar');
  if (!document.msFullscreenElement && topBar) {
    topBar.setAttribute('style', originalTopBarStyle);  // Restore the original style when exiting fullscreen
  }
});

let ws;
let currentSummaryFloor = 1;

function initWebSocket() {
  ws = new WebSocket("ws://" + location.hostname + ":81");

    ws.onopen = function () {
    console.log("WebSocket connection established");
    hideLoadingIndicator();  // Hide the loading indicator once connected
  };

  ws.onmessage = function (event) {

    const data = event.data.split(" | ");
    data.forEach((item) => {
      let parts = item.split(":");
      if (parts.length === 2) {
        let slotName = parts[0].trim();
        let status = parts[1].trim();

        let slotMap = {
          "Slot1": "F1B1",
          "Slot2": "F1B2",
          "Slot3": "F2A2",
          "Slot4": "F2A3",
          "Slot5": "F3A2",
          "Slot6": "F3A3"   
        };

        let elId = slotMap[slotName];
        if (elId) {
          let el = document.getElementById(elId);
          if (el) {
if (status === "Occupied") {
  // Handle canceling selection if it becomes occupied
  if (el.classList.contains('selected')) {
    markSpotAsOccupied(elId);  // This will remove selection + update label
  } else {
    el.classList.add("occupied"); // Just mark as occupied if not selected
  }
} else {
  el.classList.remove("occupied");
}
          }
        }
      }
    });

    const activeFloor = document.querySelector('.floor-map.active');
if (activeFloor) {
  const floorNumber = parseInt(activeFloor.id.replace('floor-', ''));

  updateFloorData(floorNumber);         // Updates the main floor map
  updateParkingSummary(floorNumber);    // Updates the summary list in real time
}

  };
}

// --- Direction map example ---
const directionsMap = {
  'F1B1': ['Enter and move left past Zone A', 'First spot in Zone B', 'Near the divider with Zone A'],
  'F1B2': ['Enter and continue forward', 'Second spot in Zone B', 'Beside F1B1'],
  'F2A2': ['Enter and move forward along Zone A', 'Second spot in Zone A', 'Beside F2A1'],
  'F2A3': ['Enter and continue into Zone A', 'Third spot in Zone A', 'Near the center of the row'],
  'F3A2': ['Enter and move forward along Zone A', 'Second spot in Zone A', 'Beside F3A1'],
  'F3A3': ['Enter and continue into Zone A', 'Third spot in Zone A', 'Near the center of the row'],
};


// --- Show directions using your HTML IDs ---
function showDirections(spotId) {
  const directions = directionsMap[spotId];
  const label = document.getElementById('selected-spot-label');
  const dirBox = document.getElementById('directions-column');

  // Update the selected spot label

  if (label) {
  label.textContent = spotId || 'Unknown Spot';
  label.style.color = (label.textContent === 'This spot is now occupied.') ? '#ff2200' : '#00ffae';
  }

  // Clear previous directions
  if (dirBox) {
    dirBox.innerHTML = '';

    if (!directions) {
      dirBox.innerHTML = '<div>No directions found.</div>';
      return;
    }

    directions.forEach(line => {
      const step = document.createElement('div');
      step.textContent = line;
      dirBox.appendChild(step);
    });
  }
}

// --- Handle spot clicks ---
function setupSpotClickHandler() {
  document.querySelectorAll('.spot').forEach(spot => {
    spot.addEventListener('click', () => {
      // Check if the spot is occupied or part of a zone
      if (spot.classList.contains('occupied') || spot.classList.contains('zone')) return;

      // Remove previous selection
      document.querySelectorAll('.spot.selected').forEach(sel => {
        sel.classList.remove('selected');
      });

      // Mark new selection
      spot.classList.add('selected');

      // Show directions
      const spotId = spot.getAttribute('id');
      showDirections(spotId);
    });
  });
}

// --- Show alert popup ---
function showSpotAlertPopup() {
  const popup = document.getElementById('spot-alert-popup');
  const container = document.querySelector('.bottom-buttons-container');
  
  if (!popup) return;

  popup.classList.remove('hidden');
  popup.classList.add('show');
  container.classList.add('show-popup'); // Add this class to expand the container

  setTimeout(() => {
    popup.classList.remove('show');
    setTimeout(() => {
      popup.classList.add('hidden');
      container.classList.remove('show-popup'); // Reset container padding
    }, 400); // Hide after fade-out
  }, 3000); // Show popup for 3 seconds
}

// --- Mark a spot as occupied ---
function markSpotAsOccupied(spotId) {
  const spot = document.getElementById(spotId);
  if (!spot) return;

  // If the spot is selected, remove the selection highlight
  if (spot.classList.contains('selected')) {
    spot.classList.remove('selected');
  }

      // Show popup alert to the user
    showSpotAlertPopup();

  // Mark as occupied
  spot.classList.add('occupied');

  // Update the label and directions if the spot was selected
  const label = document.getElementById('selected-spot-label');
  const dirBox = document.getElementById('directions-column');
  
  if (label) {
  label.textContent = 'This spot is now occupied.';
  label.style.color = '#ff2200';
 }

  if (dirBox) {
    dirBox.innerHTML = '<div>Spot is no longer available. Choose another one.</div>';
  }
}

// --- Initialize everything ---
window.addEventListener('load', setupSpotClickHandler);

// --- Layout syncing and dragging ---
function matchHeight() {
  const ref = document.querySelector('.left-right-container');
  const target = document.querySelector('.new-feature');

  if (ref && target) {
    setInterval(() => {
      target.style.height = 'auto';
      target.style.height = ref.offsetHeight + 'px';
    }, 1000); // adjust time as needed
  }
}

window.addEventListener('DOMContentLoaded', matchHeight);

function resetMapPosition(floorNumber) {
  const floorResetPositions = {
    1: { left: "-352.173px", top: "-3.76543px" },
    2: { left: "15.2346px", top: "-267px" },
    3: { left: "15.2346px", top: "-267px" },
  };

  const activeMap = document.querySelector(`#floor-${floorNumber}`);
  const pos = floorResetPositions[floorNumber];

  if (activeMap && pos) {
    activeMap.style.left = pos.left;
    activeMap.style.top = pos.top;
    activeMap.style.cursor = 'grab';
  }
}

function changeFloor(floorNumber) {
  document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
  document.querySelectorAll('.floor-map').forEach(map => map.classList.remove('active'));

  const tabs = document.querySelectorAll('.tab');
  tabs[floorNumber - 1].classList.add('active');

  const floorMap = document.getElementById('floor-' + floorNumber);
  floorMap.classList.add('active');

  // Sync the dropdown with the selected tab
  const dropdown = document.getElementById('floor-select');
  if (dropdown) {
    dropdown.value = floorNumber.toString();
  }

  resetMapPosition(floorNumber);
  updateFloorData(floorNumber);

  // Optional: If you're on the summary page, also update the parking summary
  if (typeof updateParkingSummary === 'function') {
    updateParkingSummary(floorNumber);
  }

  // Update global floor variable if used elsewhere
  currentSummaryFloor = floorNumber;
}

function enableDragging() {
  const mapContainer = document.getElementById('map-container');
  let isDragging = false;
  let startX, startY, initialX, initialY;

  function handleDown(e) {
    isDragging = true;
    const evt = e.type.includes('mouse') ? e : e.touches[0];
    const activeMap = document.querySelector('.floor-map.active');

    startX = evt.clientX;
    startY = evt.clientY;
    initialX = parseFloat(activeMap.style.left || 0);
    initialY = parseFloat(activeMap.style.top || 0);

    activeMap.style.cursor = 'grabbing';
  }

  function handleMove(e) {
    if (!isDragging) return;
    const evt = e.type.includes('mouse') ? e : e.touches[0];

    const dx = evt.clientX - startX;
    const dy = evt.clientY - startY;

    const activeMap = document.querySelector('.floor-map.active');
    activeMap.style.left = `${initialX + dx}px`;
    activeMap.style.top = `${initialY + dy}px`;
  }

  function handleUp() {
    isDragging = false;
    const activeMap = document.querySelector('.floor-map.active');
    activeMap.style.cursor = 'grab';
  }

  mapContainer.addEventListener('mousedown', handleDown);
  mapContainer.addEventListener('mousemove', handleMove);
  mapContainer.addEventListener('mouseup', handleUp);
  mapContainer.addEventListener('mouseleave', handleUp);

  mapContainer.addEventListener('touchstart', handleDown, { passive: false });
  mapContainer.addEventListener('touchmove', handleMove, { passive: false });
  mapContainer.addEventListener('touchend', handleUp);
}

function updateFloorData(floorNumber) {
  const floorLabel = document.getElementById("floorLabel");
if (floorLabel) {
  let suffix = 'th';
  if (floorNumber === 1) suffix = 'st';
  else if (floorNumber === 2) suffix = 'nd';
  else if (floorNumber === 3) suffix = 'rd';

  floorLabel.textContent = `${floorNumber}${suffix} Floor`;
}

  const floorMap = document.getElementById(`floor-${floorNumber}`);
  const allSpots = floorMap.querySelectorAll(".spot");

  const allVacantSpots = Array.from(allSpots).filter(spot =>
    !spot.classList.contains("occupied") &&
    spot.textContent.trim() !== ""
  );

  const circle = document.querySelector(".left-feature .circle");
  if (circle) {
    circle.textContent = allVacantSpots.length;
  }

  const zonePriority = ['A', 'B', 'C', 'D'];
  const topVacantSpots = allVacantSpots
    .filter(spot => {
      const label = spot.textContent.trim();
      const zone = label.split('-')[1]?.charAt(0);
      return zonePriority.includes(zone);
    })
    .sort((a, b) => {
      const aZone = a.textContent.split('-')[1][0];
      const bZone = b.textContent.split('-')[1][0];
      return zonePriority.indexOf(aZone) - zonePriority.indexOf(bZone);
    })
    .slice(0, 4);

  const labelContainer = document.querySelector(".right-feature .spot-labels");
  if (labelContainer) {
    labelContainer.innerHTML = "";
    topVacantSpots.forEach(spot => {
      const span = document.createElement("span");
      span.textContent = spot.textContent.trim();
      labelContainer.appendChild(span);
    });
  }

  const totalSpots = allSpots.length;
  const vacantCount = allVacantSpots.length;
  const circleContainer = document.querySelector(".left-feature .circle-container");
  if (circleContainer) {
    const greenPercentage = (vacantCount / totalSpots) * 100;
    const redPercentage = 100 - greenPercentage;
    circleContainer.style.background = `conic-gradient(#00ffae ${greenPercentage}%, #FF0033 ${greenPercentage}% 100%)`;
  }

    // Update the right-side summary cards with new counts
  const floor1Count = document.getElementById('floor1-count');
  const floor2Count = document.getElementById('floor2-count');
  const floor3Count = document.getElementById('floor3-count');

  if (floor1Count) floor1Count.textContent = getAllVacantSpots(1).length;
  if (floor2Count) floor2Count.textContent = getAllVacantSpots(2).length;
  if (floor3Count) floor3Count.textContent = getAllVacantSpots(3).length;

  // Returning vacant spots if needed
  return allVacantSpots; // You can access this from outside the function if necessary
}

// New function to get all vacant spots for the current floor
function getAllVacantSpots(floorNumber) {
  const floorMap = document.getElementById(`floor-${floorNumber}`);
  if (!floorMap) return [];

  const allSpots = floorMap.querySelectorAll(".spot");
  return Array.from(allSpots).filter(spot => 
    !spot.classList.contains("occupied") && spot.textContent.trim() !== ""
  );
}

// Example usage:
const vacantSpots = getAllVacantSpots();
console.log(vacantSpots);  // Logs all vacant spots for the active floor

function updateParkingSummary(floorNumber) {
  const vacantSpots = getAllVacantSpots(floorNumber);
  const parkingListContainer = document.getElementById('parking-list');
  const parkingTitle = document.querySelector('.parking-title');
  parkingListContainer.innerHTML = ''; // Clear previous spots

  if (vacantSpots.length === 0) {
    parkingTitle.textContent = 'No vacant spots available. Go to next floor!';
    return;
  }

  // Reset title and add vacant spots
  parkingTitle.textContent = 'Vacant Spots';
  vacantSpots.forEach(spot => {
    const spotItem = document.createElement('div');
    spotItem.classList.add('parking-spot');
    spotItem.textContent = spot.textContent.trim();
    parkingListContainer.appendChild(spotItem);
  });
}

function handleFloorChange(value) {
  const floorNumber = parseInt(value);
  changeFloor(floorNumber); // Reuse the main tab logic
}

window.changeFloor = changeFloor;

window.addEventListener('load', () => {
  initWebSocket();
  matchHeight();
  enableDragging();

  const initialMap = document.querySelector('.floor-map.active');
  if (initialMap) {
    initialMap.style.left = "-352.173px";
    initialMap.style.top = "-3.76543px";
    initialMap.style.cursor = "grab";
  }

  updateFloorData(1);

  ws.onerror = function (error) {
    console.error("WebSocket error:", error);
    hideLoadingIndicator();
  };

  ws.onclose = function () {
    console.log("WebSocket connection closed");
    hideLoadingIndicator();
  };
});

window.addEventListener('resize', matchHeight);
</script>

<div class="main-container active">
    <div class="tabs">
      <div class="tab main-tab active" onclick="changeFloor(1)">1st Floor</div>
      <div class="tab main-tab" onclick="changeFloor(2)">2nd Floor</div>
      <div class="tab main-tab" onclick="changeFloor(3)">3rd Floor</div>
      <div class="tab main-tab" onclick="goToSummary()">All</div>
    </div>

    <!-- <div class="tabs summary-tab">
      <div class="tab" onclick="goToSummary()">📊</div>

    </div> -->

    <div class="map-container" id="map-container">

      <!-- First Floor -->
      <div class="floor-map active" id="floor-1">
        <div class="lot">
          
          <!-- Main Portion -->
          <div class="main-portion">
            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>
            
            <div class="column">
              <div class="spot occupied" id="F1E1">F1-E1</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1E2">F1-E2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1E3">F1-E3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1E4">F1-E4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1E5">F1-E5</div>
              <div class="divider" style="height: 3px;"></div>
            </div>
            
            <div class="pathway"></div>
            
            <div class="column">
              <div class="spot occupied" id="F1D1">F1-D1</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1D2">F1-D2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1D3">F1-D3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1D4">F1-D4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1D5">F1-D5</div>
              <div class="divider" style="height: 3px;"></div>
            </div>
            
            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>
            
            <div class="column">
              <div class="spot occupied" id="F1C1">F1-C1</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1C2">F1-C2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1C3">F1-C3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1C4">F1-C4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1C5">F1-C5</div>
              <div class="divider" style="height: 3px;"></div>
            </div>
            
            <div class="pathway"></div>
            
            <div class="column">
              <div class="spot" id="F1B1">F1-B1</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot" id="F1B2">F1-B2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1B3">F1-B3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1B4">F1-B4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1B5">F1-B5</div>
              <div class="divider" style="height: 3px;"></div>
            </div>           
            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>
            <div class="column">
              
              <div class="spot occupied" id="F1A1">F1-A1</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1A2">F1-A2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1A3">F1-A3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1A4">F1-A4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F1A5">F1-A5</div>
              <div class="divider" style="height: 3px;"></div>
            </div>     
            <div class="pathway"></div>        
          </div>

          <!-- Additional Portion -->
          <div class="portion-F2">
            <div class="column">
              <div class="spot zone zoneF2-1"></div>
            </div>
            <div class="path"></div>
            <div class="column">
              <div class="spot zone zoneF2-2"></div>
            </div>
            <div class="path"></div>
            <div class="column">
              <div class="spot zone zoneF2-3"></div>
            </div>
          </div>

          <!-- Main Path -->
          <div class="main-path">
            <div class="main-pathway"></div>
          </div>
        </div>
      </div>

      <!-- Second Floor Placeholder -->
      <div class="floor-map" id="floor-2">

        <div class="lot">
                    <div class="main-path">
                      <div class="pathway-F2"></div>
                    </div>

          <!-- Main Portion -->
          <div class="main-portion">
            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>         
            <div class="column">
              <div class="spot occupied" id="F2A4">F2-A4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot" id="F2A3">F2-A3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot" id="F2A2">F2-A2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2A1">F2-A1</div>
              <div class="divider" style="height: 3px;"></div>
            </div>       

            <div class="pathway"></div>
            
            <div class="column">
              <div class="spot occupied" id="F2B4">F2-B4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2B3">F2-B3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2B2">F2-B2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2B1">F2-B1</div>
              <div class="divider" style="height: 3px;"></div>
            </div>
            
            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>
            
            <div class="column">
              <div class="spot occupied" id="F2C4">F2-C4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2C3">F2-C3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2C2">F2-C2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2C1">F2-C1</div>
              <div class="divider" style="height: 3px;"></div>
            </div>
            
            <div class="pathway"></div>
            
            <div class="column">
              <div class="spot occupied" id="F2D4">F2-D4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2D3">F2-D3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2D2">F2-D2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2D1">F2-D1</div>
              <div class="divider" style="height: 3px;"></div>
            </div>      

            <div class="divider" style="width: 3px; height:auto; background-color: white"></div>

            <div class="column">
              <div class="spot occupied" id="F2E4">F2-E4</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2E3">F2-E3</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2E2">F2-E2</div>
              <div class="divider" style="height: 3px;"></div>
              <div class="spot occupied" id="F2E1">F2-E1</div>
              <div class="divider" style="height: 3px;"></div>
            </div> 

            <div class="pathway"></div>        
          </div>

          <!-- Additional Portion -->
          <div class="portion-F2">
            <div class="column">
              <div class="spot zone zoneF2-1"></div>
            </div>
            <div class="path"></div>
            <div class="column">
              <div class="spot zone zoneF2-2"></div>
            </div>
            <div class="path"></div>
            <div class="column">
              <div class="spot zone zoneF2-3"></div>
            </div>
          </div>

          <!-- Main Path -->
          <div class="main-path">
            <div class="pathway-F2"></div>
          </div>
        </div>
      </div>

      <!-- Third Floor Placeholder -->
      <div class="floor-map" id="floor-3">
        <div class="lot">
          <div class="main-path">
            <div class="pathway-F2"></div>
          </div>
<!-- Main Portion -->
<div class="main-portion">
  <div class="divider" style="width: 3px; height:auto; background-color: white"></div>         
  <div class="column">
    <div class="spot occupied" id="F3A4">F3-A4</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot" id="F3A3">F3-A3</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot" id="F3A2">F3-A2</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3A1">F3-A1</div>
    <div class="divider" style="height: 3px;"></div>
  </div>       

  <div class="pathway"></div>
  
  <div class="column">
    <div class="spot occupied">F3-B4</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied">F3-B3</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied">F3-B2</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied">F3-B1</div>
    <div class="divider" style="height: 3px;"></div>
  </div>
  
  <div class="divider" style="width: 3px; height:auto; background-color: white"></div>
  
  <div class="column">
    <div class="spot occupied" id="F3C4">F3-C4</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3C3">F3-C3</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3C2">F3-C2</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3C1">F3-C1</div>
    <div class="divider" style="height: 3px;"></div>
  </div>
  
  <div class="pathway"></div>
  
  <div class="column">
    <div class="spot occupied" id="F3D4">F3-D4</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3D3">F3-D3</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3D2">F3-D2</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3D1">F3-D1</div>
    <div class="divider" style="height: 3px;"></div>
  </div>      

  <div class="divider" style="width: 3px; height:auto; background-color: white"></div>

  <div class="column">
    <div class="spot occupied" id="F3E4">F3-E4</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3E3">F3-E3</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3E2">F3-E2</div>
    <div class="divider" style="height: 3px;"></div>
    <div class="spot occupied" id="F3E1">F3-E1</div>
    <div class="divider" style="height: 3px;"></div>
  </div> 

  <div class="pathway"></div>        
</div>

<!-- Additional Portion -->
<div class="portion-F2">
  <div class="column">
    <div class="spot zone zoneF2-1"></div>
  </div>
  <div class="path"></div>
  <div class="column">
    <div class="spot zone zoneF2-2"></div>
  </div>
  <div class="path"></div>
  <div class="column">
    <div class="spot zone zoneF2-3"></div>
  </div>
</div>

<!-- Main Path -->
<div class="main-path">
  <div class="pathway-F2"></div>
</div>
</div>
      </div>
    </div>
  </div>

<div class="bottom-buttons-container active">
 
<div id="spot-alert-popup" class="popup hidden">
  <div class="popup-content">
    <strong>Notice:</strong> Oops! The spot you selected just got occupied. Please pick another..
  </div>
</div>

    <div class="left-right-container">
      <div class="left-feature">
        <div class="label">No. of vacant spots</div>
          <div class="circle-wrapper">
            <div class="circle-container"></div>
            <div class="circle">5</div>
          </div>
          <!-- Dynamic Floor Label inside left-feature -->
          <div class="floor-label" id="floorLabel">1st Floor</div>
      </div>
 
        <div class="right-feature">
          <div class="label">Vacant spots</div>
          <div class="spot-labels">
              <span>F1-A1</span>
              <span>F1-A2</span>
              <span>F1-A3</span>
              <span>F1-A4</span>
          </div>
          <div style="display: flex; justify-content: flex-end; gap: 20px; width: 100%; margin: 20px
          0 0; border-top: 1px solid rgba(255, 255, 255, 0.2);">
              <button style="font-size: 20px; padding: 10px; cursor: pointer; background: none; border: none; color: #bbb; outline: none; text-align: center;">&#187;</button> <!-- » -->
          </div>
      </div>
    </div>
    
  <div class="new-container">
      <div class="new-feature" style="border-radius: 12px; width: 100%; padding: 15px">
        <div class="label" style="font-size: 1rem; margin-bottom: 10px; padding-bottom: 12px;
        border-bottom: 1px solid rgba(255, 255, 255, 0.2); color: #ffffff;">Let’s Find You a Spot</div>
          
        <div class="feature-content" style="display: flex; align-items: stretch; gap: 25px;">

          <div style="flex: 1; display: flex; justify-content: center; align-items: center;">
            <div id="selected-spot-label" style="font-size: 2rem; font-weight: bold; color:
            #00ffae;">Ready to Park?</div>
          </div>
              <!-- Right Column: Directions -->
              <div id="directions-column" style="flex: 1; display: flex; flex-direction: column; justify-content: center; gap: 8px; font-size: 0.70rem; font-style: italic; color: #bbbbbb; line-height: 1.4; text-align: left;">
                <div>Select a spot to view directions</div>
              </div>
  </div>
 </div>
</div>
</div>

<div class="summary-container">
  <div class="summary-page">
    
    <!-- Top Bar -->
    <div class="top-bar">
      <div class="left-group">
        <button class="top-button back-button" onclick="goBackToMain()">Back</button>
        <div class="top-button fullscreen-button" onclick="toggleFullScreen()">⤢</div>
      </div>
      <select class="top-button dropdown" onchange="handleFloorChange(this.value)" id="floor-select">
        <option value="1">1st Floor</option>
        <option value="2">2nd Floor</option>
        <option value="3">3rd Floor</option>
      </select>
    </div>

    <!-- LEFT SECTION: Vacant Spots -->
    <div class="summary-left">
      <div class="parking-title">Vacant Spots</div>
      <div class="spot-labels summary-spots">
        <div class="parking-list" id="parking-list">
          <!-- dynamic content will appear here from JS -->
        </div>
      </div>
    </div>

    <!-- RIGHT SECTION: Floor Summary Cards -->
<div class="summary-right">
  <div class="floor-summary-card">
    <h3>1st Floor</h3>
    <p id="floor1-count" class="slot-count">2</p>
  </div>
  <div class="floor-summary-card">
    <h3>2nd Floor</h3>
    <p id="floor2-count" class="slot-count">2</p>
  </div>
  <div class="floor-summary-card">
    <h3>3rd Floor</h3>
    <p id="floor3-count" class="slot-count">2</p>
  </div>
</div>
  </div>
</div>

<!-- <div id="loading-indicator" class="loading-overlay">
  <div class="loading-spinner"></div>
  <div>Loading, please wait...</div>
</div>  -->

</body>
</html>
 )rawliteral";

// Function declarations
long readDistanceCM(int trigPin, int echoPin);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(RED_LED1, OUTPUT);
  pinMode(GREEN_LED1, OUTPUT);
  pinMode(RED_LED2, OUTPUT);
  pinMode(GREEN_LED2, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

    // Add this line to print IP
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", html_page);
  });

  server.begin();
}

unsigned long lastBroadcast = 0;

void loop() {
  webSocket.loop();

  if (millis() - lastBroadcast > 1000) {
    lastBroadcast = millis();

    long dist1 = readDistanceCM(TRIG1, ECHO1);
    parkingSlots[0] = (dist1 < 12) ? "Occupied" : "Available";

    long dist2 = readDistanceCM(TRIG2, ECHO2);
    parkingSlots[1] = (dist2 < 12) ? "Occupied" : "Available";

        // Control LEDs for Slot1
    if (parkingSlots[0] == "Occupied") {
      digitalWrite(RED_LED1, HIGH);
      digitalWrite(GREEN_LED1, LOW);
    } else {
      digitalWrite(RED_LED1, LOW);
      digitalWrite(GREEN_LED1, HIGH);
    }

    // Control LEDs for Slot2
    if (parkingSlots[1] == "Occupied") {
      digitalWrite(RED_LED2, HIGH);
      digitalWrite(GREEN_LED2, LOW);
    } else {
      digitalWrite(RED_LED2, LOW);
      digitalWrite(GREEN_LED2, HIGH);
    }

    String response = "Slot1:" + parkingSlots[0] + " | Slot2:" + parkingSlots[1] +
                      " | Slot3:" + parkingSlots[2] + " | Slot4:" + parkingSlots[3] +
                      " | Slot5:" + parkingSlots[4] + " | Slot6:" + parkingSlots[5];

    Serial.println("Broadcasting: " + response);
    webSocket.broadcastTXT(response);
  }
}

long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = String((char*)payload);
    Serial.println("Received from client " + String(num) + ": " + message);

    int startIndex = 0;
    while (startIndex < message.length()) {
      int colonIndex = message.indexOf(':', startIndex);
      if (colonIndex == -1) break;

      int pipeIndex = message.indexOf('|', colonIndex);  // not " |"
      if (pipeIndex == -1) pipeIndex = message.length(); // last one

      String slotId = message.substring(startIndex, colonIndex);
      slotId.trim();

      String status = message.substring(colonIndex + 1, pipeIndex);
      status.trim();

      if (slotId.startsWith("Slot")) {
        int slotNum = slotId.substring(4).toInt();
        if (slotNum >= 1 && slotNum <= 6) {
          parkingSlots[slotNum - 1] = status;
          Serial.println("Updated " + slotId + " to " + status);
        }
      }

      startIndex = pipeIndex + 1;
      while (startIndex < message.length() && message[startIndex] == ' ') startIndex++; // Skip extra spaces
    }
  }
} 