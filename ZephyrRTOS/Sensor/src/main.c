#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>

#define SLEEP_TIME 	1000

static struct device *sensor_device;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) 
	{
		printk("Connection failed (err %u)\n", err);
	} 
	else 
	{
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static void bt_ready(int err) 
{
   if (err)   
   {     
	   printk("bt_enable init failed with err %d\n", err);
	   return;
	}
	
	printk("Bluetooth initialised OK\n");   

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
} 

void main(void)
{
    int err;  

	printk("Sensor MkII\n");
	
 	err = bt_enable(bt_ready);

	if (err)  
	{
		printk("bt_enable failed with err %d\n", err);
	} 

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);

	if (err)  
	{
		printk("bt_unpair failed with err %d\n", err);
	} 

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	sensor_device = device_get_binding("BME280");

	if (sensor_device == NULL) {
		printk("Could not get BME280 device\n");
		return;
	}

	printk("Dev %p name %s\n", sensor_device, sensor_device->config->name);

	while (1) {
		
		struct sensor_value temp, press, humidity;

		sensor_sample_fetch(sensor_device);
		sensor_channel_get(sensor_device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(sensor_device, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(sensor_device, SENSOR_CHAN_HUMIDITY, &humidity);

		bt_gatt_bas_set_battery_level(temp.val1);

		printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
		      	temp.val1, 
				temp.val2, 
				press.val1, 
				press.val2,
		      	humidity.val1, 
			  	humidity.val2);

		k_sleep(5000);
	}
}