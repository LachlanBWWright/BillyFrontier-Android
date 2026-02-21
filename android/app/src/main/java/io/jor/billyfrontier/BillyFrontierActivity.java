package io.jor.billyfrontier;

import org.libsdl.app.SDLActivity;

public class BillyFrontierActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }

    @Override
    protected String getMainFunction() {
        return "SDL_main";
    }
}
