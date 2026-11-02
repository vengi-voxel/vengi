package io.github.vengi_voxel.voxedit;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * VoxEdit activity. SDL loads libSDL3.so and libmain.so, then calls SDL_main.
 * Packaged data files are copied into the app files directory so the existing
 * POSIX filesystem search (home path) can see them.
 */
/**
 * Stock SDL only turns MotionEvent.ACTION_UP into a mouse button release. A click on
 * the emulator is often ACTION_BUTTON_RELEASE or ACTION_HOVER_ENTER instead, so the
 * press never gets a matching up. This surface sends that release as ACTION_UP.
 */
class VoxEditSurface extends SDLSurface {
	private boolean _mouseDown;
	private boolean _mouseUpSent;

	VoxEditSurface(Context context) {
		super(context);
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		if (dropDuplicateMouseUp(event)) {
			return true;
		}
		noteMouseDown(event);
		// CANCEL on the touch stream is not an ACTION_UP, so SDL drops it.
		deliverMissingMouseUp(event, false);
		boolean handled = super.onTouch(v, event);
		if (isMouse(event) && event.getActionMasked() == MotionEvent.ACTION_UP) {
			_mouseUpSent = true;
			_mouseDown = false;
		}
		return handled;
	}

	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		noteMouseDown(event);
		deliverMissingMouseUp(event, true);
		return super.onGenericMotionEvent(event);
	}

	private boolean isMouse(MotionEvent event) {
		int index = 0;
		int action = event.getActionMasked();
		if (action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_POINTER_UP) {
			index = event.getActionIndex();
		}
		if (index < 0 || index >= event.getPointerCount()) {
			return false;
		}
		return event.getToolType(index) == MotionEvent.TOOL_TYPE_MOUSE;
	}

	private void noteMouseDown(MotionEvent event) {
		if (!isMouse(event)) {
			return;
		}
		int action = event.getActionMasked();
		if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_BUTTON_PRESS) {
			_mouseDown = true;
			_mouseUpSent = false;
		}
	}

	private boolean dropDuplicateMouseUp(MotionEvent event) {
		if (!isMouse(event) || !_mouseUpSent) {
			return false;
		}
		if (event.getActionMasked() != MotionEvent.ACTION_UP) {
			return false;
		}
		_mouseDown = false;
		return true;
	}

	private void deliverMissingMouseUp(MotionEvent event, boolean includeActionUp) {
		if (!isMouse(event) || !_mouseDown || _mouseUpSent) {
			return;
		}
		int action = event.getActionMasked();
		boolean release = action == MotionEvent.ACTION_BUTTON_RELEASE || action == MotionEvent.ACTION_CANCEL;
		if (action == MotionEvent.ACTION_HOVER_ENTER && event.getButtonState() == 0) {
			release = true;
		}
		if (includeActionUp && action == MotionEvent.ACTION_UP) {
			release = true;
		}
		if (!release) {
			return;
		}
		int index = 0;
		if (action == MotionEvent.ACTION_POINTER_UP) {
			index = event.getActionIndex();
		}
		if (index < 0 || index >= event.getPointerCount()) {
			index = 0;
		}
		SDLActivity.onNativeMouse(0, MotionEvent.ACTION_UP, event.getX(index), event.getY(index), false);
		_mouseUpSent = true;
		_mouseDown = false;
	}
}

public class VoxEdit extends SDLActivity {
	private static final String TAG = "VoxEdit";

	@Override
	protected SDLSurface createSDLSurface(Context context) {
		return new VoxEditSurface(context);
	}

	@Override
	protected String[] getArguments() {
		Intent intent = getIntent();
		if (intent == null) {
			return new String[0];
		}
		String filter = intent.getStringExtra("imgui_test_filter");
		if (filter == null || filter.isEmpty()) {
			return new String[0];
		}
		return new String[] {"--imgui_test_filter", filter};
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		extractAssets();
		super.onCreate(savedInstanceState);
	}

	private void extractAssets() {
		try {
			PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
			File dest = getFilesDir();
			File stamp = new File(dest, ".vengi-assets-stamp");
			String token = Long.toString(info.lastUpdateTime);
			if (stamp.isFile()) {
				BufferedReader reader = new BufferedReader(new FileReader(stamp));
				String have = reader.readLine();
				reader.close();
				if (token.equals(have)) {
					return;
				}
			}
			copyAssetTree(getAssets(), "", dest);
			FileWriter writer = new FileWriter(stamp);
			writer.write(token);
			writer.close();
		} catch (Exception e) {
			Log.e(TAG, "Failed to extract assets", e);
		}
	}

	private void copyAssetTree(AssetManager assets, String assetPath, File destRoot) throws IOException {
		String[] children = assets.list(assetPath);
		if (children == null || children.length == 0) {
			if (!assetPath.isEmpty()) {
				copyAssetFile(assets, assetPath, new File(destRoot, assetPath));
			}
			return;
		}
		for (String child : children) {
			String childPath = assetPath.isEmpty() ? child : assetPath + "/" + child;
			String[] grand = assets.list(childPath);
			if (grand != null && grand.length > 0) {
				copyAssetTree(assets, childPath, destRoot);
			} else {
				copyAssetFile(assets, childPath, new File(destRoot, childPath));
			}
		}
	}

	private void copyAssetFile(AssetManager assets, String assetPath, File dest) throws IOException {
		File parent = dest.getParentFile();
		if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
			throw new IOException("Could not create " + parent);
		}
		InputStream in = assets.open(assetPath);
		OutputStream out = new FileOutputStream(dest);
		byte[] buf = new byte[16384];
		int n;
		while ((n = in.read(buf)) > 0) {
			out.write(buf, 0, n);
		}
		in.close();
		out.close();
	}
}
