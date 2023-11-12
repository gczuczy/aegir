import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';

import { FontAwesomeModule} from '@fortawesome/angular-fontawesome';

import { HttpClientModule } from '@angular/common/http';

import { FormsModule, FormGroup, ReactiveFormsModule } from '@angular/forms';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { MaintenanceComponent } from './maintenance/maintenance.component';
import { SettingsComponent } from './settings/settings.component';
import { ProgramComponent } from './program/program.component';
import { AddProgramComponent } from './add-program/add-program.component';
import { EditProgramComponent } from './edit-program/edit-program.component';
import { LoadProgramComponent } from './load-program/load-program.component';
import { NgChartsModule } from 'ng2-charts';

@NgModule({
  declarations: [
    AppComponent,
    BrewComponent,
    ProgramsComponent,
    MaintenanceComponent,
    SettingsComponent,
    ProgramComponent,
    AddProgramComponent,
    EditProgramComponent,
    LoadProgramComponent
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    MatSlideToggleModule,
    AppRoutingModule,
    HttpClientModule,
    FormsModule,
    ReactiveFormsModule,
    FontAwesomeModule,
    NgChartsModule
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
