package com.severs.android.directcharacterioaccess;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import com.severs.android.lib.charactor_io.LibCharactorIO;


public class DirectCharacterIOAccess extends Activity {

  private TextView mTextView;
  private TextView mTextViewWrite;
  private TextView mTextViewRead;

  private LibCharactorIO mLibCharactorIO;;

  private Button mButtonWrite;
  private Button mButtonRead;
  private Button mButtonFlush;

  private byte[] mBufferWrite;
  private byte[] mBufferRead;

  public static final int WRITE_BUFFER_SIZE = 10;
  public static final int READ_BUFFER_SIZE = 5;

  public static String newline = System.getProperty("line.separator");

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    super.setContentView(R.layout.buffer_status);

    mLibCharactorIO = new LibCharactorIO();

    mTextView = (TextView) super.findViewById(R.id.buffer_status);

    mBufferWrite = new byte[WRITE_BUFFER_SIZE];
    for (int i = 0; i < WRITE_BUFFER_SIZE; ++i) {
      mBufferWrite[i] = (byte)i;
    }

    mButtonWrite = (Button) super.findViewById(R.id.button_write);
    mButtonWrite.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          String writeString = "";
          writeString = "write " + mBufferWrite.length + " bytes";
          mTextView.setText("pressed write button");
          for (int i = 0; i < WRITE_BUFFER_SIZE; ++i) {
            mBufferWrite[i]++;
            writeString = writeString + newline + mBufferWrite[i];
          }
          mTextViewWrite.setText(writeString);
          mLibCharactorIO.writeBuffer(mBufferWrite, 0, WRITE_BUFFER_SIZE);
          renderBufferStatus();
        }
    });
    mTextViewWrite = (TextView) super.findViewById(R.id.buffer_write);

    mButtonRead = (Button) super.findViewById(R.id.button_read);
    mButtonRead.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          mBufferRead = mLibCharactorIO.readBuffer(READ_BUFFER_SIZE, 5000);
          String readString = "";
          readString = "read " + mBufferRead.length + " bytes";
          for (int i = 0; i < mBufferRead.length; ++i) {
            readString = readString + newline + mBufferRead[i];
          }
          mTextViewRead.setText(readString);
          renderBufferStatus();
        }
    });
    mTextViewRead = (TextView) super.findViewById(R.id.buffer_read);

    mButtonFlush = (Button) super.findViewById(R.id.button_flush);
    mButtonFlush.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          mTextView.setText("pressed flush button");
          mLibCharactorIO.flushBuffer();
          renderBufferStatus();
        }
    });

  }

  private void renderBufferStatus() {
    int bufferSize = mLibCharactorIO.getBufferSize();
    int bufferCapacity = mLibCharactorIO.getBufferCapacity();
    mTextView.setText(super.getString(R.string.buffer_status, bufferSize, bufferCapacity));
  }
}