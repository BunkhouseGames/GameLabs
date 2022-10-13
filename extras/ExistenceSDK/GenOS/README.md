# GenOS

#### Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

## Large Scale Worlds Enabled by Arctic Theory

## Abstract

GenOS gives developers the tools to create massive procedurally generated game worlds including terrain, ecology, climate, wildlife and infrastructure.

These game worlds are suitable for massively multiplayer online games where it is fully interactable and can be affected by players, npc’s or other types of game simulation.

The game world evolves over time by these interactions and the result is fed back into the procedural simulation creating a positive feedback loop. In essence this means that any change to the game world that occurs during online gameplay can be re-simulated on a global scale.

![https://lh5.googleusercontent.com/-L-8DN1EoG8CI4tEMBA_oKYvauUBb_Hfrh_lEspB0aQyipfAUQ8OMB-M_iBMMD5zkZGFa0qXjcxCcn2DPYvrgNbrNTLP7mqUHr1fnGuKinCUi_CVXnF1kdG9uw8wy9agNnEulFmZ=s0](https://lh5.googleusercontent.com/-L-8DN1EoG8CI4tEMBA_oKYvauUBb_Hfrh_lEspB0aQyipfAUQ8OMB-M_iBMMD5zkZGFa0qXjcxCcn2DPYvrgNbrNTLP7mqUHr1fnGuKinCUi_CVXnF1kdG9uw8wy9agNnEulFmZ=s0)

## Details

The technology enables game developers to create large, dynamic game worlds using macro- and micro-level simulation. Macro-level simulation can change the terrain and affect the climate, while micro-level simulations involve player interactions with elements in the world such as trees and rocks, and determines dynamics such as placement, distribution, and regrowth rate of these elements in a realistic way.

![https://lh5.googleusercontent.com/T8PBRM53ye-VyVf7pJYBt3Q5tKh0DYJ3JmmQsYGZSuM0nHghbQDrvfUcOtXJVKIlRL7ZmJtI-XXG_7gM3dFagRA5MCHinIUasLIDhThhjnGrbKJO5PUet-vjpMlZTvevnFB445FC=s0](https://lh5.googleusercontent.com/T8PBRM53ye-VyVf7pJYBt3Q5tKh0DYJ3JmmQsYGZSuM0nHghbQDrvfUcOtXJVKIlRL7ZmJtI-XXG_7gM3dFagRA5MCHinIUasLIDhThhjnGrbKJO5PUet-vjpMlZTvevnFB445FC=s0)

![https://lh3.googleusercontent.com/Html3yUQUFbuem4nTaC0AKfLaBLglOPH8JkcnCbFIPxZ4C-VEle3vJeaihEWMkLaOJjhTO_fwipdyIQON-BCJfM3Jn_gbbQR6QP_cfJeDLwlzv3tAjV7vYRhn-nKTO097Z5m4cVX=s0](https://lh3.googleusercontent.com/Html3yUQUFbuem4nTaC0AKfLaBLglOPH8JkcnCbFIPxZ4C-VEle3vJeaihEWMkLaOJjhTO_fwipdyIQON-BCJfM3Jn_gbbQR6QP_cfJeDLwlzv3tAjV7vYRhn-nKTO097Z5m4cVX=s0)

### Example use case: Foresting

A forest grows on the side of a mountain. The players chop down trees to harvest wood. Trees are replenished over time but as logging continues the forest thins out and eventually disappears. This affects the wildlife, and even the weather.

### Example use case: Dam building

A small river flows in a gorge and some players decide to build a dam. They spend a few weeks gathering the required materials and build the dam. The macro level simulation determines that the flow of water has been impeded and a reservoir starts forming. As the days go by, the reservoir grows and causes changes in the vegetation and wildlife.

GenOS makes it possible to simulate ecological and geological factors, as well as dynamic weather elements that directly affect gameplay mechanics and allows the game world itself to change according to player actions and the state of the game environment.

Traditionally, multiplayer game worlds are static environments that a player inhabits where the only changes to the game happen via player input and those changes are most often non-persistent. With GenOS, the world itself can also evolve based on a variety of factors.

GenOS is developed as a turnkey solution, especially for multiplayer and massively multiplayer games, providing specialized solutions for persistent worlds. Developers can run distributed, macro-scale simulations to influence and drive evolution of a massive game world inhabited by thousands of players.

GenOS also runs micro-level simulation which involves player interaction with elements in the world such as trees and rocks, and determines dynamics such as placement, distribution, and regrowth rate of these elements in a realistic way.

The GenOS macro simulation makes it possible for the game world to feel alive in the view of the player. With continuous simulation, environments can be truly dynamic, even with players in the game. It enables complex behaviours on a macro scale, such as landscape and weather affecting plant growth, or animal migration depending on how the world changes.

GenOS makes it possible for these two simulation layers of the world to seamlessly interact with each other. The technology excels in optimized state replication, minimizing data traffic for interaction and events, while making sure information is provided in the most optimal way to all parties.

Scientifically accurate simulation of natural events such as avalanches, volcanic eruptions and flooding can be fed into the game world where they form a feedback loop with player actions allowing for unparalleled player engagement in the progression of the persistent game world itself.

The GenOS evolution cycle runs perpetually on the game assets with input from various systems and changes the game structure in fundamental ways based on player actions and any environmental factors that the game designers want. In this example implementation we see for example Houdini and AI scripts crunching on game data to power the positive feedback loop.

![https://lh3.googleusercontent.com/wtVgm6bUyniL0rQXgjwUIL-UnyKhHVNFHKSKbp_q453rimjN5XxQQSsWL_9D_bzuJZqxWbzcI6qTOGwQHzQ_Z4vW0-rPDkF8NvN9MlAb1bjupREJnczNSWUH05BCgo8loVh04S1r=s0](https://lh3.googleusercontent.com/wtVgm6bUyniL0rQXgjwUIL-UnyKhHVNFHKSKbp_q453rimjN5XxQQSsWL_9D_bzuJZqxWbzcI6qTOGwQHzQ_Z4vW0-rPDkF8NvN9MlAb1bjupREJnczNSWUH05BCgo8loVh04S1r=s0)

Here we see an example implementation of a GenOS Evolution cycle. Clients and game servers download game data from an object store. Actions within the game are stored in a database (Kamo) that is fed into the Evolution engine which runs offline from the game servers and alters the game assets and places them back in the object store.

![https://lh6.googleusercontent.com/YfSo6pHi3I0kzaSM8zqBUm8gygcpc5oTGceNEap1rP30FlVzqZAzjqVvWBE3mu3A7QTWkbViii4pfe33HJU43oBagfNT6oGsSBVDNj9JO_AlNGhdlfa5oYVYq1AJeEfu3iV0ZHrm=s0](https://lh6.googleusercontent.com/YfSo6pHi3I0kzaSM8zqBUm8gygcpc5oTGceNEap1rP30FlVzqZAzjqVvWBE3mu3A7QTWkbViii4pfe33HJU43oBagfNT6oGsSBVDNj9JO_AlNGhdlfa5oYVYq1AJeEfu3iV0ZHrm=s0)

Clients and servers download new game content automatically as needed and the cycle begins anew.

GenOS is a layered system with Evolution and Reseed steps run outside the game engine itself and it also has an Unreal plugin component which interfaces with the underlying game engine for providing the tools necessary to build and interact with GenOS features.

GenOS plugin is a set of plugins, both for the editor, server and client.
