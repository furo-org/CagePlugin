# Cage: Autonomous mobile robot simulator plugin for UE4

## Dependencies and related projects

This plugin depends on ZMQUE plugin and PxArticulationLink plugin.

+ [ZMQUE Plugin](https://github.com/furo-org/ZMQUE)
+ [PxArticulationLink Plugin](https://github.com/yosagi/PxArticulationLink)

CageClient library wraps detail of communications with CommActor of this plugin.

+ [CageClient library](https://github.com/furo-org/CageClient)

VTC project uses this plugin.

+ [VTC](https://github.com/furo-org/VTC)

## Actors in this plugin

### Cage C++/CommActor

Status reports and actor commands server.

### Cage/Sensors/Velodyne

Velodyne VLP-16 Actor.

### Cage/Vehicle/PuffinBP

Mobile robot 'Puffin' Actor.

### Cage/MapUtils/Randomizer

### Cage/Player/Pedestrian

Pedestrian pawn who wanders specified points randomly.

### Cage/Player/CagePlayer

Player pawn who can grab and follow mobile robot actors.

### Cage/Player/LidarMan

Player pawn who has Velodyne Actor on his head.

### Cage/Player/LidarManStable

Player pawn who has floating Velodyne Actor over his head.

### Cage/MapUtils/GeoReference

Reference point actor for converting UE coordinate values(x,y) to geographic coordinate values(B,L). Place this actor in the level and fill "Location" fields with corresponding geographic coordinate values.

## License

Copyright 2019 Tomoaki Yoshida <yoshida@furo.org>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

