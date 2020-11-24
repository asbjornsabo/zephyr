Microphone Control for Generic Audio
####################################

This document describes how to run the microphone control functionality,
both as a client and as a server. Note that in the examples below,
some lines of debug have been removed to make the examples shorter
and provide a better overview.

Microphone Controller (Client)
******************************

The client is able to control the input mute value of a microphone device
(server). If the server also implements Audio Input Control Service (AICS)
instances, the client can also control the individual state of those,
which makes the client able to set the gain or mute each individual audio input
on the server (microphone) device.

Using the Microphone Controller
===============================

When the btaudio stack has been initialized (:code:`btaudio init`),
and a device has been connected, the client can be initialized by
calling :code:`btaudio mics_client discover`, which will start a discovery for the MICS
UUIDs and store the handles, and optionally subscribe to all notifications
(default is to subscribe to all).

Since a server may have multiple AICS instances, the commands starting with
:code:`aics_` of the commands will take an index (starting from 0) as input.
The number of AICS instances on the server will be visible if
:code:`BT_DEBUG_MICS_CLIENT` is enabled.

The AICS control point requires a change counter value for each write.
This is handled automatically by the client implementation if it has subscribed
to the AICS state characteristic. If the change counter value is invalid on a
write (:code:`Write failed (0x80)`), it may be necessary to call
:code:`aics_read_input_state`, to update the value in the client.


Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Muting MICS:

.. code-block:: console

   uart:~$ btaudio mics_client discover
   <dbg> bt_mics_client.mics_discover_func: Setup complete for MICS
   <dbg> bt_mics_client.mics_discover_include_func: Discover include complete for MICS: 2 AICS
   <dbg> bt_mics_client.aics_discover_func: Setup complete for AICS 2 / 2
   uart:~$ btaudio mics_client mute
   <dbg> bt_mics_client.mute_notify_handler: Mute 1
   <dbg> bt_mics_client.mics_client_write_mics_mute_cb: Write successful (0x00)


Muting AICS:

.. code-block:: console

   uart:~$ btaudio mics_client aics_input_mute 0
   <dbg> bt_mics_client.aics_notify_handler: Gain 0, mute 1, mode 2, counter 21
   <dbg> bt_mics_client.mics_client_write_aics_cp_cb: Write successful (0x00)

Setting gain can only be done if the mode is manual (2) (or manual only (0)).
If the mode is automatic (3) or automatic only (1), then the server should not
update the gain value.

.. code-block:: console

   uart:~$ btaudio mics_client aics_set_gain 1 10
   <dbg> bt_mics_client.aics_notify_handler: Gain 10, mute 0, mode 2, counter 21
   <dbg> bt_mics_client.mics_client_write_aics_cp_cb: Write successful (0x00)
   uart:~$ btaudio mics_client aics_set_automatic_input_gain 1
   <dbg> bt_mics_client.aics_notify_handler: Gain 10, mute 0, mode 3, counter 22
   <dbg> bt_mics_client.mics_client_write_aics_cp_cb: Write successful (0x00)
   uart:~$ btaudio mics_client aics_set_gain 1 5
   <dbg> bt_mics_client.mics_client_write_aics_cp_cb: Write successful (0x00)


Microphone Device (Server)
**************************
The server typically resides on devices that has controllable microphones.
The server holds a muted state for the microphone(s) on the device.
If individual microphone control is needed by a remote client,
it is necessary for the MICS server to include one or more AICS instances.

Currently MICS and VCS share the same AICS instances.

Using the Microphone Device
===========================
The server itself does not expose any APIs to change the values currently.
If the MICS server includes any AICS instances, these will be controllable via
the :code:`aics` subcommands.

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt advertise on
   Advertising started
