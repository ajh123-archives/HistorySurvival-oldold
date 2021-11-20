#include <main.hpp>
#include <client_main.hpp>
#include <entt/entt.hpp>
#include <stdio.h>

extern "C" {
#include "raylib.h"
#include "raymath.h"
}

#define NOUSER
#include <net_client.hpp>

enum class CustomMsgTypes : uint32_t
{
    InvalidMessage,
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomClient : public hsc::net::client_interface<CustomMsgTypes>
{
public:
    void PingServer()
    {
        hsc::net::packets::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;

        // Caution with this...
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        send(msg);
    }

    void MessageAll()
    {
        hsc::net::packets::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::MessageAll;
        send(msg);
    }
};

int client_main(std::string addr, int port)
{
    CustomClient c;
    std::cout << "Connecting to " << addr << ":" << port << std::endl;
    c.connect(addr, port);

    if (c.isConnected()) {
        std::cout << "Connected to " << addr << ":" << port << std::endl;

        const int windowWidth = 800;
        const int windowHeight = 450;


        // Enable config flags for resizable window and vertical synchro
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(windowWidth, windowHeight, "History Survival");
        SetWindowMinSize(windowWidth, windowHeight);

        // Define the camera to look into our 3d world
        Camera camera = { 0 };
        camera.position = { 10.0f, 10.0f, 10.0f }; // Camera position
        camera.target = { 0.0f, 0.0f, 0.0f };      // Camera looking at point
        camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
        camera.fovy = 45.0f;                                // Camera field-of-view Y
        camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type


        Model tower = LoadModel("resources/models/obj/turret.obj");                 // Load OBJ model
        Texture2D texture = LoadTexture("resources/models/obj/turret_diffuse.png"); // Load model texture
        tower.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;            // Set model diffuse texture

        Vector3 towerPos = { 0.0f, 0.0f, 0.0f };                        // Set model position
        BoundingBox towerBBox = GetMeshBoundingBox(tower.meshes[0]);    // Get mesh bounding box

        // Ground quad
        Vector3 g0 = { -50.0f, 0.0f, -50.0f };
        Vector3 g1 = { -50.0f, 0.0f,  50.0f };
        Vector3 g2 = { 50.0f, 0.0f,  50.0f };
        Vector3 g3 = { 50.0f, 0.0f, -50.0f };

        // Test triangle
        Vector3 ta = { -25.0f, 0.5f, 0.0f };
        Vector3 tb = { -4.0f, 2.5f, 1.0f };
        Vector3 tc = { -8.0f, 6.5f, 0.0f };

        Vector3 bary = { 0.0f, 0.0f, 0.0f };

        // Test sphere
        Vector3 sp = { -30.0f, 5.0f, 5.0f };
        float sr = 4.0f;

        Ray ray = { 0 };                    // Picking line ray

        RayCollision collision = { 0 };

        SetCameraMode(camera, CAMERA_FREE); // Set a free camera mode


        SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
        //--------------------------------------------------------------------------------------

        // Main game loops
        while (!WindowShouldClose())        // Detect window close button or ESC key
        {
            if (!c.isConnected()) {
                break;
            }

            c.PingServer();
            if (!c.messagesToUs().empty())
            {
                auto msg = c.messagesToUs().pop_front().msg;

                switch (msg.header.id)
                {
                case CustomMsgTypes::ServerAccept:
                {
                    // Server has responded to a ping request				
                    std::cout << "Server Accepted Connection" << std::endl;
                }
                break;


                case CustomMsgTypes::ServerPing:
                {
                    // Server has responded to a ping request
                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;
                    msg >> timeThen;
                    std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << std::endl;
                }
                break;

                case CustomMsgTypes::ServerMessage:
                {
                    // Server has responded to a ping request	
                    uint32_t clientID;
                    msg >> clientID;
                    std::cout << "Hello from [" << clientID << "]" << std::endl;
                }
                break;
                }
            }

            // Update
            //----------------------------------------------------------------------------------
            Vector2 mouse = GetMousePosition();
            UpdateCamera(&camera);          // Update camera


            // Display information about closest hit
            RayCollision collision = { 0 };
            char* hitObjectName = "None";
            collision.distance = FLT_MAX;
            collision.hit = false;
            Color cursorColor = WHITE;

            // Get ray and test against objects
            ray = GetMouseRay(mouse, camera);

            // Check ray collision against ground quad
            RayCollision groundHitInfo = GetRayCollisionQuad(ray, g0, g1, g2, g3);

            if ((groundHitInfo.hit) && (groundHitInfo.distance < collision.distance))
            {
                collision = groundHitInfo;
                cursorColor = GREEN;
                hitObjectName = "Ground";
            }

            // Check ray collision against test triangle
            RayCollision triHitInfo = GetRayCollisionTriangle(ray, ta, tb, tc);

            if ((triHitInfo.hit) && (triHitInfo.distance < collision.distance))
            {
                collision = triHitInfo;
                cursorColor = PURPLE;
                hitObjectName = "Triangle";

                bary = Vector3Barycenter(collision.point, ta, tb, tc);
            }

            // Check ray collision against test sphere
            RayCollision sphereHitInfo = GetRayCollisionSphere(ray, sp, sr);

            if ((sphereHitInfo.hit) && (sphereHitInfo.distance < collision.distance))
            {
                collision = sphereHitInfo;
                cursorColor = ORANGE;
                hitObjectName = "Sphere";
            }

            // Check ray collision against bounding box first, before trying the full ray-mesh test
            RayCollision boxHitInfo = GetRayCollisionBox(ray, towerBBox);

            if ((boxHitInfo.hit) && (boxHitInfo.distance < collision.distance))
            {
                collision = boxHitInfo;
                cursorColor = ORANGE;
                hitObjectName = "Box";

                // Check ray collision against model
                // NOTE: It considers model.transform matrix!
                RayCollision meshHitInfo = GetRayCollisionModel(ray, tower);

                if (meshHitInfo.hit)
                {
                    collision = meshHitInfo;
                    cursorColor = ORANGE;
                    hitObjectName = "Mesh";
                }
            }
            //----------------------------------------------------------------------------------

            // Draw
            //----------------------------------------------------------------------------------
            // Draw everything in the render texture, note this will not be rendered on screen, yet
            BeginDrawing();
            ClearBackground(BLACK);  // Clear render texture background color

            BeginMode3D(camera);


            // Draw the tower
            // WARNING: If scale is different than 1.0f,
            // not considered by GetRayCollisionModel()
            DrawModel(tower, towerPos, 1.0f, WHITE);

            // Draw the test triangle
            DrawLine3D(ta, tb, PURPLE);
            DrawLine3D(tb, tc, PURPLE);
            DrawLine3D(tc, ta, PURPLE);

            // Draw the test sphere
            DrawSphereWires(sp, sr, 8, 8, PURPLE);

            // Draw the mesh bbox if we hit it
            if (boxHitInfo.hit) DrawBoundingBox(towerBBox, LIME);

            // If we hit something, draw the cursor at the hit point
            if (collision.hit)
            {
                DrawCube(collision.point, 0.3f, 0.3f, 0.3f, cursorColor);
                DrawCubeWires(collision.point, 0.3f, 0.3f, 0.3f, RED);

                Vector3 normalEnd;
                normalEnd.x = collision.point.x + collision.normal.x;
                normalEnd.y = collision.point.y + collision.normal.y;
                normalEnd.z = collision.point.z + collision.normal.z;

                DrawLine3D(collision.point, normalEnd, RED);
            }

            EndMode3D();

            // for (int i = 0; i < 10; i++) DrawRectangle(0, (gameScreenHeight/10)*i, gameScreenWidth, gameScreenHeight/10, colors[i]);

            DrawText("If executed inside a window,\nyou can resize the window,\nand see the screen scaling!", 10, 25, 20, WHITE);
            DrawText(TextFormat("Default Mouse: [%i , %i]", (int)mouse.x, (int)mouse.y), 350, 25, 20, GREEN);

            EndDrawing();
            //--------------------------------------------------------------------------------------
        }

        // De-Initialization
        //--------------------------------------------------------------------------------------

        CloseWindow();                      // Close window and OpenGL context
        //--------------------------------------------------------------------------------------
    }
    else {
        std::cout << "Server Down"<<std::endl;
        return -1;
    }
    return 0;
}

