# Kamo

ELEVATOR PITCH

## Configuring 3rd Party Stuff

### HiRedis Library
 
* Get the latest release as a zip file from here: https://github.com/redis/hiredis/releases
* Unzip the files into `kamo/Source/ThirdParty/hiredis`  (Do not include the root folder from the zip file)
* Windows:
  * Change the build settings to generate a **static library*** instead of a **.dll**. Edit `CMakeLists.txt` and replace `SHARED` with `STATIC` in the *ADD_LIBRARY* definition.
  * Run `cmake CMakeLists.txt`
  * Run `MSBuild.exe hiredis.vcxproj /p:Configuration=Release` and `MSBuild.exe hiredis.vcxproj /p:Configuration=Debug`
* Lunix:
  * Run `make`
  * Run `cp libhiredis.a Release/`
  

### Redis++ Library

The source code can be fetched as zip file from https://github.com/sewenew/redis-plus-plus/tags

Unzip `/src/sw/redis++` into `kamo/Source/ThirdParty/redis++`.

Additinally get the [RedLock recipes code branch](https://github.com/sewenew/redis-plus-plus/archive/recipes.zip) and extract the *recipes* folder into `redis++/recipes`.


## Suite

* [Kamoflage](https://github.com/directivegames/kamo-frontend) - Web app
* [Kamo REST](https://github.com/directivegames/kamo-backend) - REST Api
* [WokOn](https://github.com/directivegames/kamo-client/tree/develop/DriftMMOPlugin-Client/Plugins/KamoEditorWidgets) - World creation tool
* [Kamo Demo Project](https://github.com/directivegames/kamo-client) - Viking World demo project



## Adding Kamo to an Unreal Project

This is a small guide on how to add Kamo to the first person template project in unreal engine.  Going through this tutorial will result in the FirstPersonShooter plugin having simple persistance through Kamo

### Setup
- Create a new unreal project using the FirstPersonShooter C++ template through Unreal Engine ( with starter content )
- Clone the Kamo plugin into the plugins folder <ProjectRoot>/Plugin/Kamo
- Compile and run the project

## Project Configuration

The first thing we need to do is to the initial configuration for Kamo.  We need to>
- Create the Kamo Class Datatable
- Trigger Kamo Initialization through the game logic

### Kamo Class Table
The Kamo Class table is a datatable asset that can be created anywhere in the project ( right click in the content browser and select Miscellaneous/Datatable.  This will pop up a message asking us to select the data structure for our datatable.  Select KamoClassMap

We will add data to the table in a seperate step

### Initialize Kamo
The Kamo plugin provides an actor component that handles the kamo initialization.  Create a blueprint class from the project gamemode and add the "BP_KamoManager" actor component to it.  Then select the BP_KamoManager component in the game mode and connect the "Kamo Classes Datatable" to the data table asset that was created in the previous step.

This concludes the configuration so all that is left now is connecting it to the game logic.

## Persisting Data
The FirstPersonShooter Template has a few actors that can be persisted.  For this demo we will connect the FirstPersonProjectile blueprint to kamo so that any projectiles that the player shoots will stay in the world.

Open the Kamo Class Datatable and add a new entry
- Row name is the kamo class name which needs to be a unique and descriptive name.  Name it Projectile
- Kamo Actor Class is a class reference that connects to the unreal class.  Connect it to FirstPersonProjectile
- Kamo Class is a class that determines how the actor class is handled by kamo.  Connect it to: KamoActor

When the game is played and the projectiles are fired,  kamo automatically handles tracking the position of these projectiles.  Shooting a few projectiles, shutting of the game and playing again will result in all the projectiles still being in the world.

Kamo runs in file mode by default where the database is stored on the filesystem.  The default location is <User>/.kamo/default/

''Notes on projectile life time''
The projectiles in the FirstPersonShooter template are set to destroy themselves after 3 seconds.  For the best result its best to disable this which makes the projectiles stay forever in the world.  This can be done by opening up the FirstPersonProjectile,  going to class defaults/Actor/InitialLifeSpawn and setting that to 0.

