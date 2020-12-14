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

Using the volume control client
===============================

When the stack has been initialized (:code:`init`),
and a device has been connected, the volume control client can discover VCS on
the connected device calling :code:`vcs_client discover`, which will
start a discovery for the VCS UUIDs and store the handles and subscribe to all
notifications.

Since a server may have multiple included Volume Offset or Audio Input service
instances, some of the client commands commands will take an index
(starting from 0) as input.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Decrease volume:

.. code-block:: console

   uart:~$ vcs_client discover
   VCS discover done with 1 AICS
   uart:~$ vcs_client state_get
   VCS volume 100, mute 0
   uart:~$ vcs_client volume_down
   VCS volume 99, mute 0
   VCS vol_down done


Mute an AICS instance:

.. code-block:: console

   uart:~$ vcs_client aics_input_state_get 0
   AICS index 0 state gain 0, mute 0, mode 2
   uart:~$ vcs_client aics_input_mute 0
   AICS index 0 state gain 0, mute 1, mode 2
   Muted index 0


Set the offset of a VOCS instance:

.. code-block:: console

   uart:~$ vcs_client vocs_state_get 0
   VOCS index 0 offset 0
   vcs_client vocs_offset_set 0 50
   VOCS index 0 offset 50
   Offset set for index 0


Volume Control Service (VCS)
****************************
The volume control service is a service that typically resides on devices that
have inputs or outputs.


Using the volume control service
================================
The VCS server shall be initialized with :code:`vcs init` before use, as it
dynamically added to the server. The VCS API allows changing or reading all the
same values as the client, and can be used without a connection.

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ vcs init

Modifying and reading values
----------------------------

.. code-block:: console

   uart:~$ vcs state_get
   VCS volume 99, mute 0
   uart:~$ vcs volume_set 120
   VCS volume 120, mute 0
   uart:~$ vcs aics_gain_setting_get 0
   AICS index 0 gain settings units 1, min -100, max 100
   uart:~$ vcs aics_gain_set 0 50
   AICS index 0 state gain 50, mute 1, mode 2
