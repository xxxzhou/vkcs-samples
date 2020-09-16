package vulkan.samples.vkcs1;

import android.app.NativeActivity;
import android.os.Bundle;

public class MainActivity extends NativeActivity {
    static {
        // Load native library
        // System.loadLibrary("vkcs1");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}