

<script type='text/javascript'>
function gobutton() {
    var URL = "/api/plugin-apis/sendconfig/true";
    $.get(URL);
}

</script>
<style>
</style>


<div id="global" class="settings">
<fieldset>
<legend>FPP ZCPP Plugin</legend>

<h3>Loaded ZCPP Controller Configs</h3>
<div>
 <?php
 $fp = fopen("/home/fpp/media/config/fpp-zcpp-plugin", "r") or die("error");
 $tableControllers = array();
 while (!feof($fp)){
  $line = fgets($fp);
  if (strlen($line) > 1) { 
  list($ip, $size) = explode(",", $line);
  $tableControllers[] = array($ip, $size);
  }
 }
 fclose($fp);
 print "<table border='1' cellspacing='0' cellpadding='3'>";
 print "<tr><td>IP</td><td>Size</td></tr>";
 foreach($tableControllers as $controller)
 {
	  print "<tr><td>{$controller[0]}</td><td>{$controller[1]}</td></tr>";
 }
  print "</table>";
 
 ?>
</div>

<h4>About</h4>
<p>
The FPP ZCPP plugin will send the ZCPP Config data to the controller based on the xLights ZCPP Files.
<p>

</fieldset>
</div>
