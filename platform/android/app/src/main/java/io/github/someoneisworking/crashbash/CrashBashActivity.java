package io.github.someoneisworking.crashbash;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Locale;

public final class CrashBashActivity extends Activity {
    private static final int PICK_GAME_INPUT = 1001;
    private static final String PREFERENCES = "crashbash_install";
    private static final String INSTALLED = "installed";
    private static final String INPUT_URI = "input_uri";
    private static final String INSTALL_DIRECTORY = "game";
    private static final String[] ACCEPTED_SUFFIXES = {
        ".chd", ".iso", ".zip"
    };

    private static final boolean NATIVE_RUNTIME_AVAILABLE;

    static {
        boolean loaded = false;
        try {
            System.loadLibrary("main");
            loaded = true;
        } catch (UnsatisfiedLinkError ignored) {
            // Debug shell builds deliberately exercise setup without pretending the game runtime exists.
        }
        NATIVE_RUNTIME_AVAILABLE = loaded;
    }

    private TextView status;
    private Button play;

    private static native boolean nativeValidateAndInstall(
        int fileDescriptor,
        String displayName,
        long byteCount,
        String installDirectory
    );

    private static native void nativeStartGame(String installDirectory);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        showSetupScreen();
    }

    private void showSetupScreen() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setGravity(Gravity.CENTER_HORIZONTAL);
        content.setPadding(dp(32), dp(32), dp(32), dp(32));
        content.setBackgroundColor(Color.rgb(19, 27, 58));

        TextView title = text(getString(R.string.setup_title), 28);
        title.setTextColor(Color.WHITE);
        content.addView(title, matchWidth());

        TextView explanation = text(getString(R.string.setup_explanation), 17);
        explanation.setTextColor(Color.rgb(214, 222, 242));
        content.addView(explanation, spacedWidth());

        status = text("", 15);
        status.setTextColor(Color.rgb(255, 211, 79));
        content.addView(status, spacedWidth());

        Button choose = new Button(this);
        choose.setText(R.string.choose_game_file);
        choose.setOnClickListener(view -> chooseGameInput());
        content.addView(choose, spacedWidth());

        play = new Button(this);
        play.setText(R.string.play);
        play.setOnClickListener(view -> startInstalledGame());
        content.addView(play, spacedWidth());

        Button forget = new Button(this);
        forget.setText(R.string.forget_selection);
        forget.setOnClickListener(view -> forgetSelection());
        content.addView(forget, spacedWidth());

        setContentView(content);
        refreshState();
    }

    private void chooseGameInput() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(
            Intent.EXTRA_MIME_TYPES,
            new String[] {
                "application/octet-stream",
                "application/zip",
                "application/x-chd",
                "application/x-iso9660-image"
            }
        );
        startActivityForResult(intent, PICK_GAME_INPUT);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_GAME_INPUT || resultCode != RESULT_OK || data == null) {
            return;
        }
        Uri uri = data.getData();
        if (uri == null) {
            status.setText(R.string.selection_unreadable);
            return;
        }
        validateSelection(uri);
    }

    private void validateSelection(Uri uri) {
        Selection selection = inspect(uri);
        if (selection == null) {
            status.setText(R.string.selection_unreadable);
            return;
        }
        if (!hasAcceptedSuffix(selection.displayName)) {
            status.setText(R.string.selection_wrong_type);
            return;
        }
        if (!NATIVE_RUNTIME_AVAILABLE) {
            status.setText(R.string.native_runtime_missing);
            return;
        }

        File installDirectory = new File(getFilesDir(), INSTALL_DIRECTORY);
        try (ParcelFileDescriptor descriptor = getContentResolver().openFileDescriptor(uri, "r")) {
            if (descriptor == null || !nativeValidateAndInstall(
                descriptor.getFd(),
                selection.displayName,
                selection.byteCount,
                installDirectory.getAbsolutePath()
            )) {
                status.setText(R.string.selection_identity_mismatch);
                return;
            }
        } catch (IOException | SecurityException error) {
            status.setText(R.string.selection_unreadable);
            return;
        }

        // Validation copies the complete install into app-private storage while this Activity still
        // owns the descriptor. The URI is retained as provenance, not as a runtime dependency.
        preferences().edit()
            .putBoolean(INSTALLED, true)
            .putString(INPUT_URI, uri.toString())
            .apply();
        refreshState();
    }

    private Selection inspect(Uri uri) {
        ContentResolver resolver = getContentResolver();
        try (Cursor cursor = resolver.query(
            uri,
            new String[] {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE},
            null,
            null,
            null
        )) {
            if (cursor == null || !cursor.moveToFirst()) {
                return null;
            }
            int nameColumn = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
            int sizeColumn = cursor.getColumnIndex(OpenableColumns.SIZE);
            if (nameColumn < 0 || sizeColumn < 0 || cursor.isNull(sizeColumn)) {
                return null;
            }
            String displayName = cursor.getString(nameColumn);
            long byteCount = cursor.getLong(sizeColumn);
            if (displayName == null || displayName.isBlank() || byteCount <= 0) {
                return null;
            }
            return new Selection(displayName, byteCount);
        } catch (RuntimeException error) {
            return null;
        }
    }

    private boolean hasAcceptedSuffix(String name) {
        String lower = name.toLowerCase(Locale.ROOT);
        for (String suffix : ACCEPTED_SUFFIXES) {
            if (lower.endsWith(suffix)) {
                return true;
            }
        }
        return false;
    }

    private void refreshState() {
        boolean installed = preferences().getBoolean(INSTALLED, false);
        play.setEnabled(installed && NATIVE_RUNTIME_AVAILABLE);
        if (installed) {
            status.setText(R.string.selection_ready);
        } else if (!NATIVE_RUNTIME_AVAILABLE) {
            status.setText(R.string.native_runtime_missing);
        } else {
            status.setText(R.string.selection_needed);
        }
    }

    private void startInstalledGame() {
        if (!preferences().getBoolean(INSTALLED, false) || !NATIVE_RUNTIME_AVAILABLE) {
            refreshState();
            return;
        }
        nativeStartGame(new File(getFilesDir(), INSTALL_DIRECTORY).getAbsolutePath());
    }

    private void forgetSelection() {
        if (!deleteInstallTree(new File(getFilesDir(), INSTALL_DIRECTORY))) {
            status.setText(R.string.reset_failed);
            return;
        }
        preferences().edit().clear().apply();
        refreshState();
    }

    private boolean deleteInstallTree(File installDirectory) {
        Path appFiles = getFilesDir().toPath().toAbsolutePath().normalize();
        Path target = installDirectory.toPath().toAbsolutePath().normalize();
        if (!appFiles.equals(target.getParent())) {
            return false;
        }
        if (!Files.exists(target)) {
            return true;
        }
        try {
            Files.walkFileTree(target, new SimpleFileVisitor<>() {
                @Override
                public FileVisitResult visitFile(Path file, BasicFileAttributes attributes)
                    throws IOException {
                    Files.delete(file);
                    return FileVisitResult.CONTINUE;
                }

                @Override
                public FileVisitResult postVisitDirectory(Path directory, IOException error)
                    throws IOException {
                    if (error != null) {
                        throw error;
                    }
                    Files.delete(directory);
                    return FileVisitResult.CONTINUE;
                }
            });
            return true;
        } catch (IOException | SecurityException error) {
            return false;
        }
    }

    private SharedPreferences preferences() {
        return getSharedPreferences(PREFERENCES, MODE_PRIVATE);
    }

    private TextView text(String value, int sizeSp) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(sizeSp);
        return view;
    }

    private LinearLayout.LayoutParams matchWidth() {
        return new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
    }

    private LinearLayout.LayoutParams spacedWidth() {
        LinearLayout.LayoutParams params = matchWidth();
        params.topMargin = dp(18);
        return params;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static final class Selection {
        final String displayName;
        final long byteCount;

        Selection(String displayName, long byteCount) {
            this.displayName = displayName;
            this.byteCount = byteCount;
        }
    }
}
