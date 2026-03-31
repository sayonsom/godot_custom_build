plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.test.godotdemo"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.test.godotdemo"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    packaging {
        jniLibs {
            // Avoid conflicts with libc++_shared.so from the AAR.
            pickFirsts += "**/*.so"
        }
    }
}

dependencies {
    // The custom Godot AAR with Lottie + AccessKit Android.
    implementation(files("libs/godot-lib-4.6.1-custom.aar"))
}
