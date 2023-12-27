use std::io::{Read, Stdout, Write};
use std::path::PathBuf;
use std::{io, thread};
use std::time::Duration;
use crossbeam::channel::{Receiver, Sender, unbounded};
use crossterm::cursor::{MoveTo, MoveToColumn, RestorePosition, SavePosition};
use crossterm::cursor;
use crossterm::{event, execute, QueueableCommand, style};
use crossterm::event::poll;
use crossterm::style::{Print, Stylize};
use crossterm::terminal::{Clear, ClearType, disable_raw_mode, enable_raw_mode};
use dialoguer::Select;
use rusty_audio::Audio;
use serial2::SerialPort;

fn select_port() -> anyhow::Result<PathBuf> {
    let available = SerialPort::available_ports()?;
    let names = available.iter().map(|x| x.display()).collect::<Vec<_>>();
    let selected = if available.len() == 0 {
        println!("Only one option, selecting {}", names[0]);
        0
    } else {
        Select::new()
            .with_prompt("Select serial port")
            .items(&names)
            .interact()?
    };

    Ok(available.into_iter().nth(selected).unwrap())
}

#[derive(Clone, Debug)]
enum Event {
    BoardAdded(usize, String),
    Unlock(usize),
    Lock(usize),
    PlaySound(usize, usize),
    Reset,
    Quit,
}

fn main() -> anyhow::Result<()> {
    let x = real_main();
    disable_raw_mode()?;
    x
}

fn real_main() -> anyhow::Result<()> {
    let path = select_port()?;
    let serial = SerialPort::open(path, 9600)?;
    let (send_main, receive_main) = unbounded();
    let (send_ui, receive_ui) = unbounded();
    let ui = thread::spawn(move || {
        ui(receive_main, send_ui).expect("UI thread panicked");
    });

    let mut audio = Audio::new();
    audio.add("0", "correct.wav");
    audio.add("1", "fail.wav");
    audio.add("2", "unlock.wav");
    if let Err(e) = device(receive_ui, send_main, serial, audio) {
        println!("{e}");
    }

    ui.join().map_err(|_| anyhow::Error::msg("UI thread panicked"))?;

    Ok(())
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
enum EspMessageType {
    SetState,
    Login,
    PlaySound,
    Reset,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct EspMessage {
    id: u8,
    message_type: EspMessageType,
    data: u32,
}


fn message_as_u8_slice(p: &EspMessage) -> &[u8] {
    unsafe {
        core::slice::from_raw_parts(
            (p as *const EspMessage) as *const u8,
            ::core::mem::size_of::<EspMessage>(),
        )
    }
}

fn message_as_mut_u8_slice(p: &mut EspMessage) -> &mut [u8] {
    unsafe {
        core::slice::from_raw_parts_mut(
            (p as *mut EspMessage) as *mut u8,
            ::core::mem::size_of::<EspMessage>(),
        )
    }
}

fn device(receive: Receiver<Event>, send: Sender<Event>, mut serial: SerialPort, mut audio: Audio) -> anyhow::Result<()> {

    let mut message = EspMessage {
        id: 0,
        message_type: EspMessageType::Login,
        data: 0,
    };
    serial.write(message_as_u8_slice(&message))?;
    serial.set_read_timeout(Duration::from_millis(100))?;
    loop {
        while serial.read_exact(message_as_mut_u8_slice(&mut message)).is_err() {
            if let Ok(x) = receive.try_recv() {
                match x {
                    Event::Unlock(x) => {
                        let message = EspMessage {
                            id: x as u8,
                            message_type: EspMessageType::SetState,
                            data: 0,
                        };
                        serial.write_all(message_as_u8_slice(&message))?;
                    }
                    Event::Lock(x) => {
                        let message = EspMessage {
                            id: x as u8,
                            message_type: EspMessageType::SetState,
                            data: 1,
                        };
                        serial.write_all(message_as_u8_slice(&message))?;
                    }
                    Event::Quit => {
                        return Ok(())
                    }
                    Event::Reset => {
                        let message = EspMessage {
                            id: 0,
                            message_type: EspMessageType::Reset,
                            data: 0,
                        };
                        serial.write_all(message_as_u8_slice(&message))?;
                    }
                    _ => {}
                }
            }
        }
        match message.message_type {
            EspMessageType::SetState => {
                if message.data == 0 {
                    send.send(Event::Unlock(message.id as usize))?;
                } else {
                    send.send(Event::Lock(message.id as usize))?;
                }
            }
            EspMessageType::Login => {
                let name = message.data.to_le_bytes().iter().take_while(|x| **x != 0).map(|c| *c as char).collect();
                send.send(Event::BoardAdded(message.id as usize, name))?;
            }
            EspMessageType::PlaySound => {
                audio.play(&message.data.to_string());
            }
            _ => {}
        }


    }
}

struct Board {
    name: String,
    locked: bool,
}

fn refresh_ui(stdout: &mut Stdout, boards: &[Option<Board>; 16]) -> anyhow::Result<()> {
    stdout.queue(SavePosition)?
        .queue(Clear(ClearType::All))?
        .queue(MoveTo(1, 0))?;

    let mut count = 0;

    for (index, board) in boards.iter().enumerate().filter_map(|(i, x)| x.as_ref().map(|v| (i, v))) {
        stdout
            .queue(Print(format!("{}: {} is ", index, board.name)))?
            .queue(
                style::PrintStyledContent(if board.locked { "LOCKED\n".red() } else { "UNLOCKED\n".green() })
            )?
            .queue(MoveToColumn(1))?;
        count += 1;
    }
    if count == 0 {
        stdout.queue(Print("No boards found\n".red()))?;
    }
    stdout.queue(RestorePosition)?;
    stdout.flush()?;

    Ok(())
}

fn ui(receive: Receiver<Event>, send: Sender<Event>) -> anyhow::Result<()> {
    let mut boards : [Option<Board>; 16]  = std::array::from_fn(|_| None);
    let mut stdout = io::stdout();
    enable_raw_mode()?;
    execute!(
        stdout,
        Clear(ClearType::All),
    )?;

    loop {
        let event = receive.try_recv();
        if let Ok(event) = event {
            match event {
                Event::BoardAdded(x, v) => {
                    boards[x] = Some(Board {
                        name: v,
                        locked: true,
                    })
                }
                Event::Unlock(x) => {
                    if let Some(v) = &mut boards[x] {
                        v.locked = false;
                    }
                }
                Event::Lock(x) => {
                    if let Some(v) = &mut boards[x] {
                        v.locked = true;
                    }
                }
                Event::PlaySound(_, v) => {
                    //println!("Playing sound {v}");
                }
                Event::Reset => {
                    for i in &mut boards {
                        *i = None;
                    }
                }
                Event::Quit => return Ok(()),
            }

            refresh_ui(&mut stdout, &boards)?;
        }

        while poll(Duration::from_secs(0))? {
            let input = event::read()?;
            if !matches!(input, event::Event::Key(event::KeyEvent { kind: event::KeyEventKind::Release, .. })) {
                match input {
                    event::Event::Key(event::KeyEvent { code: event::KeyCode::Enter, .. }) => {
                        let y = cursor::position()?.1;
                        if let Some(x) = &mut boards[y as usize] {
                            if x.locked {
                                send.send(Event::Unlock(y as usize))?;
                            } else {
                                send.send(Event::Lock(y as usize))?;
                            }
                            x.locked = !x.locked;
                            refresh_ui(&mut stdout, &boards)?;
                        }
                    }
                    event::Event::Key(x) => {
                        match x.code {
                            event::KeyCode::Up => {
                                execute!(stdout, cursor::MoveUp(1))?;
                            }
                            event::KeyCode::Down => {
                                execute!(stdout, cursor::MoveDown(1))?;
                            }
                            event::KeyCode::Char('q') => {
                                send.send(Event::Quit)?;
                                return Ok(());
                            }
                            event::KeyCode::Char('r') => {
                                send.send(Event::Reset)?;
                                for i in &mut boards {
                                    *i = None;
                                }
                                refresh_ui(&mut stdout, &boards)?;
                            }
                            _ => {},
                        }
                    }
                    _ => {}
                }
            }
        }

        thread::sleep(Duration::from_millis(10));
    }
}
