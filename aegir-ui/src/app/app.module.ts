import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpClientModule } from '@angular/common/http';
import { RouterModule } from '@angular/router';
import { APP_BASE_HREF } from '@angular/common';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';
import {NgbModule} from '@ng-bootstrap/ng-bootstrap';

import { BrewModule } from './brew/brew.module';
import { ProgramsModule } from './programs/programs.module';
import { SettingsModule } from './settings/settings.module';
import { MaintenanceModule } from './maintenance/maintenance.module';

import { AppComponent } from './app.component';

import { ApiService } from './api.service';

import { APP_ROUTES } from './app.routes';

@NgModule({
    declarations: [
	AppComponent,
    ],
    imports: [
	BrowserModule,
	FontAwesomeModule,
	HttpClientModule,
	BrewModule,
	SettingsModule,
	ProgramsModule,
	MaintenanceModule,
	RouterModule.forRoot(APP_ROUTES),
    ],
    providers: [
	ApiService
    ],
    bootstrap: [AppComponent]
})
export class AppModule { }
