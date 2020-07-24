

<script type='text/javascript'>
function gobutton() {
    var URL = "/api/plugin-apis/SendConfig/true";
    $.get(URL);
}

</script>
<style>
</style>


<div id="global" class="settings">
<fieldset>
<legend>FPP ZCPP Plugin</legend>

<input type="button" value="Send Configs" class="button" id="go" oninput="gobutton()" >


<p>
<p>
The FPP ZCPP plugin will send the ZCPP Config data to the controller based on the xLights Files.
<p>

</fieldset>
</div>
