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

When the Bluetooth stack has been initialized (:code:`bt init`),
and a device has been connected, the client can initiate discovery by
calling :code:`mics_client discover`, which will start a discovery for the MICS
UUIDs, store the handles and subscribe to all notifications.

Since a server may have multiple AICS instances, the commands starting with
:code:`aics_` of the commands will take an index (starting from 0) as input.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Muting MICS:

.. code-block:: console

   uart:~$ mics_client discover
   MICS discover done with 1 AICS
   uart:~$ mics_client mute
   Mute value 1


Muting AICS:

.. code-block:: console

   uart:~$ mics_client aics_input_mute 0
   AICS index 0 state gain 0, mute 1, mode 2
   Muted index 0


Setting gain can only be done if the mode is manual (2) (or manual only (0)).
If the mode is automatic (3) or automatic only (1), then the server should not
update the gain value.

.. code-block:: console
   uart:~$ mics_client aics_gain_set 0 10
   AICS index 0 state gain 10, mute 1, mode 2
   Gain set for index 0


Microphone Device (Server)
**************************
The server typically resides on devices that has controllable microphones.
The server holds a muted state for the microphone(s) on the device.
If individual microphone control is needed by a remote client,
it is necessary for the MICS server to include one or more AICS instances.

Using the Microphone Device
===========================
The MICS server shall be initialized with :code:`mics init` before use, as it
dynamically added to the server. The VCS API allows changing or reading all the
same values as the client, and can be used without a connection.

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ mics init

Modifying and reading values
----------------------------

.. code-block:: console

   uart:~$ mics mute_get
   Mute value 1
   uart:~$ mics unmute
   Mute value 0
   uart:~$ mics aics_gain_setting_get 0
   AICS index 0 gain settings units 1, min -100, max 100
   uart:~$ mics aics_gain_set 0 50
   AICS index 0 state gain 50, mute 1, mode 2
