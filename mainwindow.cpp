#include "mainwindow.h"
#include "ui_mainwindow.h"


void setSets(QPushButton *button, QString image_name){
    button->setFixedWidth(100);
    button->setFixedHeight(100);
    button->setStyleSheet("QPushButton {"
                          "    border: 5px solid #8f8f91;"
                          "    border-radius: 15px;"
                          "    padding: 6px;"
                          "    border-image: url(:"+image_name+ ".png);"
                                         "    background-color: #1F9999;"
                                         "}"
                                         "QPushButton:hover {"
                                         "    background-color: #26B5B5;"
                                         "}"
                                         "QPushButton:pressed {"
                                         "    background-color: #1B8383;"
                                         "}");

}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->first_column->setContentsMargins(0,10,0,0);
    ui->second_column->setContentsMargins(0,10,0,0);
    ui->first_column->setStyleSheet("font-family: JetBrains Mono NL; font-size: 18px; color: black;border: 5px solid #54D3AF;"
                                    "    border-radius: 15px;"
                                    "    padding: 6px;"
                                    "    background-color: #54D3AF;");
    ui->second_column->setStyleSheet("font-family: JetBrains Mono NL; font-size: 18px; color: black;border: 5px solid #54D3AF;"
                                     "    border-radius: 15px;"
                                     "    padding: 6px;"
                                     "    background-color: #54D3AF;");


    setSets(ui->CPU,"CPU");
    setSets(ui->RAM,"RAM");
    setSets(ui->GPU,"GPU");
    setSets(ui->MONITOR,"MONITOR");
    setSets(ui->BASEBOARD,"BASEBOARD");
    setSets(ui->DISK,"DISK");

}


MainWindow::~MainWindow()
{
    delete ui;
}

void Clear_set_text(QLabel *f_label,QLabel *s_label, QString &first_column,  QString &second_column,QStringList &arguments){
    f_label->setText(first_column);
    s_label->setText(second_column);
    arguments.clear();
    first_column.clear();
    second_column.clear();
}

void set_text(QLabel *section_name,const QString &name ){
    section_name->setText(name);
}


void MainWindow::split(QString &result){

    QTextStream stream(&result);
    while(!stream.atEnd()){
        currentLine=stream.readLine();
        found = currentLine.indexOf(":");
        if(found!=-1){
            first_column.append(currentLine.left(found) + "\n");
            second_column.append(currentLine.mid(found+1) + "\n");
        }
        else{
            first_column.append(currentLine + "\n");
            second_column.append(currentLine + "\n");
        }

    }
}

QString MainWindow::runCommand(const QString& command) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QStringList arguments;
    arguments << "-c" << command;
    process.start("bash", arguments);
    process.waitForFinished();
    QString result = QString::fromUtf8(process.readAll());
    return result;
}


void MainWindow::on_CPU_clicked()
{

    arguments << "-E" << "model name|stepping|microcode|cpu MHz|cache size" << "/proc/cpuinfo";
    process.start("grep", arguments);
    process.waitForFinished();
    QString result = QString::fromUtf8(process.readAllStandardOutput());

    result.replace("\t", "");



    QProcess scalingProcess;
    scalingProcess.start("bash", QStringList() << "-c" <<
                                     "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
    scalingProcess.waitForFinished();
    QString scalingResult = QString::fromUtf8(scalingProcess.readAllStandardOutput());

    QTextStream scalingStream(&scalingResult);
    QString scalingMHz, maxMHz, minMHz;
    scalingStream >> scalingMHz >> maxMHz >> minMHz;

    maxMHz = QString::number(maxMHz.toDouble() / 1000, 'f', 2);
    minMHz = QString::number(minMHz.toDouble() / 1000, 'f', 2);

    QString cacheInfo =
        "\nCaches (sum of all):\n"
        "  L1d: 128 KiB (4 instances)\n"
        "  L1i: 128 KiB (4 instances)\n"
        "  L2: 1 MiB (4 instances)\n"
        "  L3: 6 MiB (1 instance)\n";

    QString additionalInfo;
    additionalInfo.append("CPU max MHz: " + maxMHz + "\n");
    additionalInfo.append("CPU min MHz: " + minMHz + "\n");

    QProcess dmidecodeProcess;
    dmidecodeProcess.start("sudo", QStringList() << "dmidecode" << "-t" << "processor");
    dmidecodeProcess.waitForFinished();
    QString dmidecodeResult = QString::fromUtf8(dmidecodeProcess.readAllStandardOutput());

    QString socketDesignation, voltage;
    QTextStream dmidecodeStream(&dmidecodeResult);
    while (!dmidecodeStream.atEnd()) {
        QString line = dmidecodeStream.readLine();
        if (line.contains("Socket Designation")) {
            socketDesignation = line;
            socketDesignation.replace("Socket Designation", "Socket");
        }
        if (line.contains("Voltage")) {
            voltage = line;
        }
    }

    additionalInfo.append(socketDesignation + "\n");
    additionalInfo.append(voltage + "\n");

    int coreCount = result.count("model name");
    additionalInfo.append("Number of cores: " + QString::number(coreCount) + "\n");
    additionalInfo.append(cacheInfo);

    QString firstCoreInfo;
    QTextStream stream(&result);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        firstCoreInfo.append(line + "\n");
        if (line.contains("cache size")) {
            break;
        }
    }

    firstCoreInfo.append(additionalInfo);
    firstCoreInfo.replace("\t", "");
    split(firstCoreInfo);

    Clear_set_text(ui->first_column, ui->second_column, first_column, second_column, arguments);
    set_text(ui->section_name, "CENTRAL PROCESSING UNIT");
}




void MainWindow::on_RAM_clicked()
{

    arguments << "--type" << "17";
    process.start("dmidecode", arguments);
    process.waitForFinished();

    QByteArray result = process.readAll();
    QStringList lines = QString(result).split('\n');

    QString filteredOutput;
    foreach (const QString &line, lines) {
        if ((line.contains("Size") ||
             line.contains("Configured Voltage") ||
             line.contains("Form Factor") ||
             (line.contains("Locator")&& !line.contains("Bank Locator"))  ||
             line.contains("Type Details") ||
             line.contains("Memory Speed") ||
             line.contains("Manufacturer")||
             line.contains("Memory Type")))

        {
            QString processedLine = line;
            processedLine.replace("Configured Memory Speed", "Memory Speed");
            filteredOutput.append(processedLine + "\n");
        }
    }

    filteredOutput.replace("\t", "");

    found = filteredOutput.indexOf("Voltage: 1.2 V");

    while (found != std::string::npos) {
        filteredOutput.insert(found+14, "\n");
        found = filteredOutput.indexOf("Voltage: 1.2 V", found + 1);
    }
    split(filteredOutput);
    Clear_set_text(ui->first_column,ui->second_column,first_column,second_column,arguments);
    set_text(ui->section_name,"RANDOM ACCESS MEMORY");
}



void MainWindow::on_GPU_clicked()
{

    if(gpu_first_column.isEmpty() && gpu_second_column.isEmpty())
    {

        process.setProcessChannelMode(QProcess::MergedChannels);
        arguments << "-c" << "video";

        process.start("lshw", arguments);
        process.waitForFinished();

        QByteArray result = process.readAll();
        QStringList lines = QString(result).split('\n');

        QString filteredOutput;
        foreach (const QString &line, lines) {
            if (line.contains("description") || line.contains("product") || line.contains("vendor")
                ||line.contains("width") || line.contains("clock"))
            {
                filteredOutput.append(line + "\n");
            }
        }

        QTextStream stream(&filteredOutput);

        while(!stream.atEnd()){
            currentLine=stream.readLine();
            found =currentLine.indexOf(":");
            if(found!=-1){
                gpu_first_column.append( currentLine.left(found)+"\n");
                gpu_second_column.append( currentLine.mid(found+1)+"\n");
            }
            else{
                gpu_first_column.append( currentLine + "\n");
                gpu_second_column.append( currentLine + "\n");
            }
        }

        gpu_first_column.replace(" ","");

        QFile gpuFreqFile("/sys/class/drm/card0/gt_cur_freq_mhz");
        if (gpuFreqFile.exists() && gpuFreqFile.open(QIODevice::ReadOnly)) {
            QByteArray freqData = gpuFreqFile.readAll();
            QString freqString = QString::fromUtf8(freqData).trimmed();
            gpu_first_column.append("GPU Frequency:\n");
            gpu_second_column.append(" " + freqString + " MHz\n");
        }

        QFile gpuMaxFreqFile("/sys/class/drm/card0/gt_max_freq_mhz");
        if (gpuMaxFreqFile.exists() && gpuMaxFreqFile.open(QIODevice::ReadOnly)) {
            QByteArray maxFreqData = gpuMaxFreqFile.readAll();
            QString maxFreqString = QString::fromUtf8(maxFreqData).trimmed();
            gpu_first_column.append("Max GPU Frequency:\n");
            gpu_second_column.append(" " + maxFreqString + " MHz\n");
        }
    }


    ui->first_column->setText(gpu_first_column);
    ui->second_column->setText(gpu_second_column);
    arguments.clear();
    set_text(ui->section_name,"GRAPHICS PROCESSING UNIT");
}


void MainWindow::on_MONITOR_clicked()
{


    QString Model = runCommand("sudo ddcutil detect | grep -E \"Model\"");
    QRegularExpression re("\\s+");
    Model.replace(re, " ");
    Model = Model.trimmed();
    QStringList Model_resolution;
    Model_resolution.append(Model.split(":")[1].trimmed());
    QString monitor = Model_resolution.join("\n");

    QString Res_Freq = runCommand("xrandr | awk 'NR==3 {gsub(\"[+*]\", \"\"); print $0}'");
    Res_Freq = Res_Freq.trimmed();
    QStringList Res_Freq_resolution = Res_Freq.split(" ");
    Res_Freq_resolution.removeAll("");
    monitor.append("\n" + Res_Freq_resolution.join("\n"));

    QString Dimensions = runCommand(R"(xrandr | sed -n 's/.* \([0-9]*mm x [0-9]*mm\).*$/\1/p')");
    Dimensions = Dimensions.trimmed();
    Dimensions = Dimensions.simplified();
    Dimensions.remove("\n");
    monitor.append("\n" + Dimensions);

    QStringList Dimensions_res = Dimensions.split(" ");
    Dimensions_res.removeAll("");
    for (int i = 0; i < Dimensions_res.size(); ++i) {
        Dimensions_res[i] = Dimensions_res[i].remove(QRegularExpression("[^0-9]"));
        }
    Dimensions_res.removeAll("");
    double diagonal_mm = std::sqrt(std::pow(Dimensions_res[0].toInt(), 2) + std::pow(Dimensions_res[1].toInt(), 2));
    double diagonal_in_inches = diagonal_mm / 25.4;
    double ppi = std::ceil(sqrt(pow(1920,2)+pow(1080,2))/diagonal_in_inches);
    monitor.append("\n" + QString::number(ppi));

    first_column.append("Model name: \nMonitor resolution:  \nFrequency:  \n\n\n\nDimensions:  \nPixels per inch:");
    Clear_set_text(ui->first_column,ui->second_column,first_column,monitor,arguments);
    set_text(ui->section_name,"DISPLAY");

}


void MainWindow::on_BASEBOARD_clicked()
{
    process.setProcessChannelMode(QProcess::MergedChannels);
    arguments << "-t" << "baseboard";
    process.start("dmidecode", arguments);
    process.waitForFinished();

    QByteArray result = process.readAll();
    QStringList lines = QString(result).split('\n');
    QString filteredOutput;
    foreach (const QString &line, lines) {
        if (line.contains("Manufacturer") || line.contains("Product Name") || line.contains("Version")
            ||line.contains("Serial Number"))
        {
            filteredOutput.append(line + "\n");
        }
    }

    split(filteredOutput);
    first_column.replace("\t","");

    Clear_set_text(ui->first_column,ui->second_column,first_column,second_column,arguments);
    set_text(ui->section_name,"BASEBOARD");

}



void MainWindow::on_DISK_clicked()
{


    QProcess process;
    process.start("bash", QStringList() << "-c" <<
                              "sudo smartctl -a /dev/nvme0n1 | "
                              "grep -E 'Model Number:|Critical Warning:|SMART overall-health self-assessment test result:|Temperature:|Percentage Used:|Power-On Hours:' | "
                              "awk '/Model Number:/ {print \"Model:\", $3, $4} /Critical Warning:/ {print \"Critical Warning:\", $3} /SMART overall-health self-assessment test result:/ {print \"Health Status:\", $6} /Temperature:/ {print \"Temperature:\", $2, $3} /Percentage Used:/ {print \"Percentage Used:\", $3} /Power-On Hours:/ {print \"Power-On Hours:\", $3}' && "
                              "sudo smartctl -a /dev/nvme0n1 | grep 'Capacity' | awk '{print \"Capacity:\", $5, $6}' | sed 's/\\[\\|\\]//g'");

    if (!process.waitForFinished()) {
        qDebug() << "Failed to execute command: " << process.errorString();
        return;
    }

    QString Disk_result = process.readAllStandardOutput();
    split(Disk_result);
    first_column.replace("\t","");

    Clear_set_text(ui->first_column,ui->second_column,first_column,second_column,arguments);
    set_text(ui->section_name,"DISK");
}

