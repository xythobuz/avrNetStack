<html><head>
<title>ENC28J60 Transmit Status Vector Interpreter</title>
</head><body>
<h1>ENC28J60 Transmit Status Vector Interpreter</h1>
<p>This small PHP Tool allows you to convert the ENC28J60 Transmit Status Vector into a human-readable format. Some field will be marked green if their value seems okay, red if it's value indicates an error.</p>
<form method="get">
Status Vector Hexdump: <input type="text" name="v" value="<? echo $_GET['v']; ?>">
<input type="submit" value="Go!">
</form>
<?
if (isset($_GET['v'])) {
	$v = str_replace("0x", "", $_GET['v']);
	$v = str_replace(" ", "", $v);

	if (strlen($v) != 14) {
		echo "<h2>No valid Status Vector!</h2>\n";
	} else {
		echo "<h3>Valid Status Vector!</h3>\n";
		echo "<table border=\"1\"><tr><th>Field</th><th>Value</th><th>Description</th></tr>";

		// Bit 0-15: Transmit Byte Count
		echo "<tr><td>Transmit Byte Count</td><td";
		if (hexdec(substr($v, 2, 2).substr($v, 0, 2)) > 1518) {
			echo " bgcolor=\"#FF0000\"";
		} else {
			echo " bgcolor=\"#00FF00\"";
		}
		echo ">";
		echo hexdec(substr($v, 2, 2).substr($v, 0, 2));
		echo "</td><td>Total bytes in frame not counting collided bytes.</td></tr>\n";

		// Bit 16-19: Transmit Collision Count
		echo "<tr><td>Transmit Collision Count</td><td";
		if (hexdec(substr($v, 5, 1)) > 0) {
			echo " bgcolor=\"#FF0000\"";
		} else {
			echo " bgcolor=\"#00FF00\"";
		}
		echo ">";
		echo hexdec(substr($v, 5, 1));
		echo "</td><td>Number of collisions the current packet incurred during transmission attempts. It applies to successfully transmitted packets and as such, will not show the possible maximum count of 16 collisions.</td></tr>\n";

		// Bit 20: Transmit CRC Error
		echo "<tr><td>Transmit CRC Error</td><td";
		if ((hexdec(substr($v, 4, 1)) & 0x01) != 0) {
			echo " bgcolor=\"#FF0000\">Yes";
		} else {
			echo " bgcolor=\"#00FF00\">No";
		}
		echo "</td><td>The attached CRC in the packet did not match the internally generated CRC.</td></tr>\n";

		// Bit 21: Transmit Length Check Error
		echo "<tr><td>Transmit Length Check Error</td><td";
		if ((hexdec(substr($v, 4, 1)) & 0x02) != 0) {
			echo " bgcolor=\"#FF0000\">Yes";
		} else {
			echo " bgcolor=\"#00FF00\">No";
		}
		echo "</td><td>Indicates that frame length field value in the packet does not match the actual data byte length and is not a type field. MACON3.FRMLNEN must be set to get this error.</td></tr>\n";

		// Bit 22: Length out of Range
		echo "<tr><td>Length Out Of Range</td><td";
		if ((hexdec(substr($v, 4, 1)) & 0x04) != 0) {
			echo " bgcolor=\"#FF0000\">Yes";
		} else {
			echo " bgcolor=\"#00FF00\">No";
		}
		echo "</td><td>Indicates that frame type/length field was larger than 1500 bytes (type field).</td></tr>\n";

		// Bit 23: Transmit Done
		echo "<tr><td>Transmit Done</td><td";
		if ((hexdec(substr($v, 4, 1)) & 0x08) != 0) {
			echo " bgcolor=\"#00FF00\">Yes";
		} else {
			echo " bgcolor=\"#FF0000\">No";
		}
		echo "</td><td>Transmission of the packet was completed.</td></tr>\n";

		// Bit 24: Transmit Multicast
		echo "<tr><td>Transmit Multicast</td><td>";
		if ((hexdec(substr($v, 7, 1)) & 0x01) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Packets destination address was a Multicast address.</td></tr>\n";

		// Bit 25: Transmit Broadcast
		echo "<tr><td>Transmit Broadcast</td><td>";
		if ((hexdec(substr($v, 7, 1)) & 0x02) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Packets destination address was a Broadcast address.</td></tr>\n";

		// Bit 26: Transmit Packet Defer
		echo "<tr><td>Transmit Packet Defer</td><td>";
		if ((hexdec(substr($v, 7, 1)) & 0x04) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Packet was deferred for at least one attempt but less than an excessive defer.</td></tr>\n";

		// Bit 27: Transmit Excessive Defer
		echo "<tr><td>Transmit Excessive Defer</td><td>";
		if ((hexdec(substr($v, 7, 1)) & 0x08) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Packet was deferred in excess of 24287 bit times (2.4287ms).</td></tr>\n";

		// Bit 28: Transmit Excessive Collision
		echo "<tr><td>Transmit Excessive Collision</td><td";
		if ((hexdec(substr($v, 6, 1)) & 0x01) != 0) {
			echo " bgcolor=\"#FF0000\">Yes";
		} else {
			echo " bgcolor=\"#00FF00\">No";
		}
		echo "</td><td>Packet was aborted after the number of collisions exceeded the retransmission maximum (MACLCON1).</td></tr>\n";

		// Bit 29: Transmit Late Collision
		echo "<tr><td>Transmit Late Collision</td><td";
		if ((hexdec(substr($v, 6, 1)) & 0x02) != 0) {
			echo " bgcolor=\"#FF0000\">Yes";
		} else {
			echo " bgcolor=\"#00FF00\">No";
		}
		echo "</td><td>Collision occured beyond the collision window (MACLCON2).</td></tr>\n";

		// Bit 30: Transmit Giant
		echo "<tr><td>Transmit Giant</td><td>";
		if ((hexdec(substr($v, 6, 1)) & 0x04) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Byte count for frame was greater than MAMXFL.</td></tr>\n";

		// Bit 31: Transmit Underrun
		echo "<tr><td>Always &quot;No&quot;</td><td";
		if ((hexdec(substr($v, 6, 1)) & 0x08) != 0) {
			echo " bgcolor=\"#FF0000\">";
			echo "Yes";
		} else {
			echo " bgcolor=\"#00FF00\">";
			echo "No";
		}
		echo "</td><td>Transmit Underrun - Reserved. This bit will always be '0'.</td></tr>\n";

		// Bit 32-47: Total Bytes Transmitted on Wire
		echo "<tr><td>Total Bytes Transmitted on Wire</td><td";
		if (hexdec(substr($v, 10, 2).substr($v, 8, 2)) > 1518) {
			echo " bgcolor=\"#FF0000\"";
		} else {
			echo " bgcolor=\"#00FF00\"";
		}
		echo ">";
		echo hexdec(substr($v, 10, 2).substr($v, 8, 2));
		echo "</td><td>Total bytes transmitted on the wire for the current packet, including all bytes from collided attempts.</td></tr>\n";

		// Bit 48: Transmit Control Frame
		echo "<tr><td>Transmit Control Frame</td><td>";
		if ((hexdec(substr($v, 13, 1)) & 0x01) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>The frame transmitted was a control frame.</td></tr>\n";

		// Bit 49: Transmit Pause Control Frame
		echo "<tr><td>Transmit Pause Control Frame</td><td>";
		if ((hexdec(substr($v, 13, 1)) & 0x02) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>The frame transmitted was a control frame with a valid pause opcode.</td></tr>\n";

		// Bit 50: Backpressure applied
		echo "<tr><td>Backpressure Applied</td><td>";
		if ((hexdec(substr($v, 13, 1)) & 0x03) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Carrier sense method backpressure was previously applied.</td></tr>\n";

		// Bit 51: Transmit VLAN Tagged Frame
		echo "<tr><td>Transmit VLAN Frame</td><td>";
		if ((hexdec(substr($v, 13, 1)) & 0x04) != 0) {
			echo "Yes";
		} else {
			echo "No";
		}
		echo "</td><td>Frames length/type field contained 8100h which is the VLAN protocol identifier.</td></tr>\n";

		// Bit 52-55: Zero
		echo "<tr><td>Always Zero</td><td";
		if (hexdec(substr($v, 12, 1)) != 0) {
			echo " bgcolor=\"#FF0000\"";
		} else {
			echo " bgcolor=\"#00FF00\"";
		}
		echo ">";
		echo hexdec(substr($v, 12, 1));
		echo "</td><td>0</td></tr>\n";
?>
</table>
<?
	}
}
?>
<p>2012, <a href="http://www.xythobuz.org">Thomas Buck</a>&lt;xythobuz@xythobuz.org&gt;. Part of <a href="https://github.com/xythobuz/avrNetStack">avrNetStack</a>. Licensed under <a href="http://www.gnu.org/licenses/gpl-3.0.html">GPLv3</a>.
</body></html>
