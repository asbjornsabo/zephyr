Volume Control for Generic Audio Content Control
################################################

This document describes how to run the volume control functionality, both as
a client and as a (volume control service (VCS)) server. Note that in the
examples below, some lines of debug have been removed to make this shorter
and provide a better overview.

Volume Control Service Client
*****************************

The volume control client will typically be phones or laptops, but may also be
resource restricted devices such as a simply remote control. The client can
control the volume state of one or more outputs and one or more inputs on a
server.

The control point contains a change counter value, which needs to be known
with the client, before the client may modify the volume state. If any
request to the volume state fails due to error code 0x80 for VCS/AICS/VOCS, then
perform a new state read (e.g. :code:`read_volume_state` for VCS).

It is necessary to have :code:`BT_DEBUG_VCS_CLIENT` enabled for using the VCS
client interactively.

Using the volume control client
===============================

When the btaudio stack has been initialized (:code:`btaudio init`),
and a device has been connected, the volume control client can discover VCS on
the connected device calling :code:`btaudio vcs_client discover`, which will
start a discovery for the VCS UUIDs and store the handles, and optionally
subscribe to all notifications (default is to subscribe to all).

Since a server may have multiple included Volume Offset or Audio Input service
instances, some of the client commands commands will take an index
(starting from 0) as input.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Decrease volume:

.. code-block:: console

   uart:~$ btaudio vcs_client discover
   <dbg> bt_vcs_client.primary_discover_func: Primary discover complete
   <dbg> bt_vcs_client.vcs_discover_func: Setup complete for VCS
   <dbg> bt_vcs_client.vcs_discover_include_func: Discover include complete for VCS: 1 AICS and 1 VOCS
   <dbg> bt_vcs_client.aics_discover_func: Setup complete for AICS 1 / 1
   bt_vcs_client.vocs_discover_func: Setup complete for VOCS 1 / 1
   uart:~$ btaudio vcs_client read_volume_state
   <dbg> bt_vcs_client.vcs_client_read_volume_state_cb: Volume 255, mute 0, counter 1
   uart:~$ btaudio vcs_client volume_down
   [00:36:48.805,408] <dbg> bt_vcs_client.vcs_notify_handler: Volume 254, mute 0, counter 11
   [00:36:48.805,408] <dbg> bt_vcs_client.vcs_client_write_vcs_cp_cb: err: 0x00

Mute an AICS instance:

.. code-block:: console

   uart:~$ btaudio vcs_client aics_read_input_state 0
   <dbg> bt_aics_client.aics_client_read_input_state_cb: Index 0: err: 0x00
   <dbg> bt_aics_client.aics_client_read_input_state_cb: Gain 0, mute 0, mode 2, counter 20
   uart:~$ btaudio vcs_client aics_input_mute 0
   <dbg> bt_aics_client.aics_client_notify_handler: Index 0: Gain 0, mute 1, mode 2, counter 21
   <dbg> bt_aics_client.aics_client_write_aics_cp_cb: Index 0: err: 0x00


Set the offset of a VOCS instance:

.. code-block:: console

   uart:~$ btaudio vcs_client vocs_read_offset_state 0
   <dbg> bt_vcs_client.vcs_client_vocs_read_offset_state_cb: Index 0: err: 0x00
   <dbg> bt_vcs_client.vcs_client_vocs_read_offset_state_cb: Offset 0, counter 15
   uart:~$ btaudio vcs_client vocs_set_offset 0 50
   <dbg> bt_vcs_client.vocs_notify_handler: Index 0: Offset 50, counter 16
   <dbg> bt_vcs_client.vcs_client_write_vocs_cp_cb: Index 0: err: 0x00


Volume Control Service (VCS)
****************************
The volume control service is a service that typically resides on devices that
have inputs or outputs.

It is necessary to have :code:`BT_DEBUG_VCS` enabled for using the VCS server
interactively.

Using the volume control service
================================
The VCS server have very limited controls currently, and may only set the status
and mode of AICS (if applicable).

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt advertise on
   Advertising started

When connected
--------------

Setting AICS status

.. code-block:: console

   uart:~$ btaudio vcs aics_diactivate 0
   <dbg> bt_aics.bt_aics_deactivate: Status was set to deactivated
